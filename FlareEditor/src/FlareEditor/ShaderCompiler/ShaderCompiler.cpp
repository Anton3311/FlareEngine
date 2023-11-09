#include "ShaderCompiler.h"

#include "Flare/AssetManager/AssetManager.h"
#include "FlareEditor/AssetManager/EditorShaderCache.h"
#include "FlareEditor/ShaderCompiler/ShaderSourceParser.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <fstream>
#include <string>
#include <sstream>

namespace Flare
{
    struct PreprocessedShaderProgram
    {
        std::string Source;
        ShaderStageType Stage;
    };

    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:
        virtual shaderc_include_result* GetInclude(const char* requested_source,
            shaderc_include_type type,
            const char* requesting_source,
            size_t include_depth)
        {
            std::filesystem::path requestingPath = requesting_source;
            std::filesystem::path parent = requestingPath.parent_path();
            std::filesystem::path includedFilePath = parent / requested_source;

            if (!std::filesystem::exists(includedFilePath))
                includedFilePath = std::filesystem::path("assets/Shaders/") / requested_source;

            shaderc_include_result* includeData = new shaderc_include_result();
            includeData->content = nullptr;
            includeData->content_length = 0;
            includeData->source_name = nullptr;
            includeData->source_name_length = 0;
            includeData->user_data = nullptr;

            std::ifstream inputStream(includedFilePath);

            std::string_view path;
            if (inputStream.is_open())
            {
                std::string pathString = includedFilePath.string();
                path = pathString;

                char* sourceName = new char[path.size() + 1];
                sourceName[path.size()] = 0;

                memcpy_s(sourceName, path.size() + 1, pathString.c_str(), pathString.size());

                includeData->source_name = sourceName;
                includeData->source_name_length = pathString.size();
            }

            if (inputStream.is_open())
            {
                inputStream.seekg(0, std::ios::end);
                size_t size = inputStream.tellg();
                char* source = new char[size + 1];
                std::memset(source, 0, size + 1);

                inputStream.seekg(0, std::ios::beg);
                inputStream.read(source, size);

                includeData->content = source;
                includeData->content_length = strlen(source);
            }
            else
            {
                std::string_view message = "File not found.";

                includeData->content = message.data();
                includeData->content_length = message.size();
            }

            return includeData;
        }

        virtual void ReleaseInclude(shaderc_include_result* data)
        {
            if (data->source_name)
            {
                delete data->source_name;
                delete data->content;
            }

            delete data;
        }
    };

    static std::optional<std::vector<uint32_t>> CompileVulkanGlslToSpirv(const std::string& path, const std::string& source, shaderc_shader_kind programKind)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetGenerateDebugInfo();
        options.SetIncluder(CreateScope<ShaderIncluder>());
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(source.c_str(), source.size(), programKind, path.c_str(), "main", options);
        if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            FLARE_CORE_ERROR("Failed to compile Vulkan GLSL '{0}'", path);
            FLARE_CORE_ERROR("Shader Error: {0}", shaderModule.GetErrorMessage());
            return {};
        }

        return std::vector<uint32_t>(shaderModule.cbegin(), shaderModule.cend());
    }

    static std::optional<std::vector<uint32_t>> CompileSpirvToGlsl(const std::string& path, const std::vector<uint32_t>& spirvData, shaderc_shader_kind programKind)
    {
        spirv_cross::CompilerGLSL glslCompiler(spirvData);
        std::string glsl = glslCompiler.compile();

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetAutoMapLocations(true);
        options.SetGenerateDebugInfo();
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(glsl, programKind, path.c_str(), options);
        if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            FLARE_CORE_ERROR("Failed to compile shader '{0}'", path);
            FLARE_CORE_ERROR("Shader Error: {0}", shaderModule.GetErrorMessage());
            return {};
        }

        return std::vector<uint32_t>(shaderModule.cbegin(), shaderModule.cend());
    }

    static bool Preprocess(const std::filesystem::path& shaderPath,
        std::string_view source,
        std::vector<PreprocessedShaderProgram>& programs,
        ShaderFeatures& features)
    {
        ShaderSourceParser parser(shaderPath, source);
        parser.Parse();

        for (const auto& sourceBlock : parser.GetSourceBlocks())
        {
            PreprocessedShaderProgram& program = programs.emplace_back();
            program.Source = sourceBlock.Source;
            program.Stage = sourceBlock.Stage;
        }

        const Block& rootBlock = parser.GetBlock(0);
        for (const auto& element : rootBlock.Elements)
        {
            if (element.Name.Value == "Culling")
                features.Culling = CullingModeFromString(element.Value.Value);
            else if (element.Name.Value == "BlendMode")
            {
                auto blending = BlendModeFromString(element.Value.Value);
                if (blending)
                    features.Blending = *blending;
            }
            else if (element.Name.Value == "DepthTest")
                features.DepthTesting = element.Value.Value == "true";
            else if (element.Name.Value == "DepthFunction")
            {
                auto depthFunction = DepthComparisonFunctionFromString(element.Value.Value);
                if (depthFunction)
                    features.DepthFunction = *depthFunction;
            }
        }

        return true;
    }
    
    bool ShaderCompiler::Compile(AssetHandle shaderHandle)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

        const std::filesystem::path& shaderPath = AssetManager::GetAssetMetadata(shaderHandle)->Path;
        std::string sourceString = "";
        {
            std::ifstream file(shaderPath);
            if (!file.is_open())
            {
                FLARE_CORE_ERROR("Could not read file {0}", shaderPath.string());
                return false;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            sourceString = buffer.str();
        }

        std::string pathString = shaderPath.string();
        std::vector<PreprocessedShaderProgram> programs;

        ShaderFeatures shaderFeatures;
        std::string_view source = sourceString;
        if (!Preprocess(shaderPath, source, programs, shaderFeatures))
        {
            FLARE_CORE_ERROR("Failed to compile shader '{}'", pathString);
            return false;
        }
        
        for (const PreprocessedShaderProgram& program : programs)
        {
            shaderc_shader_kind shaderKind = (shaderc_shader_kind)0;
            switch (program.Stage)
            {
            case ShaderStageType::Vertex:
                shaderKind = shaderc_vertex_shader;
                break;
            case ShaderStageType::Pixel:
                shaderKind = shaderc_fragment_shader;
                break;
            }

            std::optional<std::vector<uint32_t>> compiledVulkanShader = ShaderCacheManager::GetInstance()->FindCache(
                shaderHandle,
                ShaderTargetEnvironment::Vulkan,
                program.Stage);

            if (!compiledVulkanShader)
            {
                compiledVulkanShader = CompileVulkanGlslToSpirv(pathString, program.Source, shaderKind);
                if (!compiledVulkanShader)
                    return false;

                ShaderCacheManager::GetInstance()->SetCache(shaderHandle, ShaderTargetEnvironment::Vulkan, program.Stage, compiledVulkanShader.value());
            }

            std::optional<std::vector<uint32_t>> compiledOpenGLShader = ShaderCacheManager::GetInstance()->FindCache(
                shaderHandle,
                ShaderTargetEnvironment::OpenGL,
                program.Stage);

            if (!compiledOpenGLShader)
            {
                compiledOpenGLShader = CompileSpirvToGlsl(pathString, compiledVulkanShader.value(), shaderKind);
                if (!compiledOpenGLShader)
                    return false;

                ShaderCacheManager::GetInstance()->SetCache(shaderHandle, ShaderTargetEnvironment::OpenGL, program.Stage, compiledOpenGLShader.value());
            }

            ((EditorShaderCache*)ShaderCacheManager::GetInstance().get())->SetShaderFeatures(shaderHandle, shaderFeatures);
        }

        return true;
    }
}
