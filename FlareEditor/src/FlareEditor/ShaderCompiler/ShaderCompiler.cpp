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

            if (std::string_view(requested_source)._Starts_with("Packages"))
            {
                includedFilePath = std::filesystem::absolute(std::filesystem::path("../") / requested_source);
            }

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

                // NOTE: Include result owns the shader path string
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
        options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            options.AddMacroDefinition("OPENGL");
            break;
        case RendererAPI::API::Vulkan:
            //options.AddMacroDefinition("VULKAN");
            break;
        }

        shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(source.c_str(), source.size(), programKind, path.c_str(), "main", options);
        if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::string_view stageName = "";
            switch (programKind)
            {
            case shaderc_vertex_shader:
                stageName = "Vertex";
                break;
            case shaderc_fragment_shader:
                stageName = "Pixel";
                break;
            }

            FLARE_CORE_ERROR("Failed to compile Vulkan GLSL '{}' Stage = {}", path, stageName);
            FLARE_CORE_ERROR("Shader Error: {0}", shaderModule.GetErrorMessage());
            return {};
        }

        return std::vector<uint32_t>(shaderModule.cbegin(), shaderModule.cend());
    }

    struct CrossCompilationOptions
    {
        shaderc_shader_kind ProgramKind;
        uint32_t LocationBase;
    };

    static uint32_t CountRegistersUsedByStruct(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& structType)
    {
        const uint32_t fieldAlignment = 16; // in OpenGL everything is aligned to 16 bytes

        size_t structSize = compiler.get_declared_struct_size(structType);
        return (uint32_t)((structSize + fieldAlignment - 1) / fieldAlignment);
    }

    static std::optional<std::vector<uint32_t>> CompileSpirvToGlsl(const std::string& path, const std::vector<uint32_t>& spirvData, const CrossCompilationOptions& options)
    {
        Scope<spirv_cross::CompilerGLSL> glslCompiler = CreateScope<spirv_cross::CompilerGLSL>(spirvData);

        {
            uint32_t location = options.LocationBase;
            spirv_cross::Compiler crossCompiler(spirvData.data(), spirvData.size());

            const auto& resources = crossCompiler.get_shader_resources();
            auto updateResourcesLocations = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources, bool isStruct = false)
            {
                for (const auto& resource : resources)
                {
                    glslCompiler->set_decoration(resource.id, spv::DecorationLocation, location);

                    uint32_t registers = 1;
                    if (isStruct)
                    {
                        const spirv_cross::SPIRType& structType = crossCompiler.get_type(resource.base_type_id);
                        registers = CountRegistersUsedByStruct(crossCompiler, structType);
                    }

                    location += registers;
                }
            };

            updateResourcesLocations(resources.sampled_images);
            updateResourcesLocations(resources.push_constant_buffers, true);
        }

        std::string glsl = glslCompiler->compile();

        shaderc::Compiler compiler;
        shaderc::CompileOptions compilerOptions;
        compilerOptions.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
        compilerOptions.SetSourceLanguage(shaderc_source_language_glsl);
        compilerOptions.SetGenerateDebugInfo();
        compilerOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

        shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(glsl, options.ProgramKind, path.c_str(), compilerOptions);
        if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::string_view stageName = "";
            switch (options.ProgramKind)
            {
            case shaderc_vertex_shader:
                stageName = "Vertex";
                break;
            case shaderc_fragment_shader:
                stageName = "Pixel";
                break;
            }

            FLARE_CORE_ERROR("Failed to compile shader '{}' Stage = {}", path, stageName);
            FLARE_CORE_ERROR("Shader Error: {0}", shaderModule.GetErrorMessage());
            FLARE_CORE_TRACE(glsl);
            return {};
        }

        return std::vector<uint32_t>(shaderModule.cbegin(), shaderModule.cend());
    }

    static void PrintError(const ShaderError& error)
    {
        if (error.Position.Line == UINT32_MAX || error.Position.Column == UINT32_MAX)
            FLARE_CORE_ERROR(error.Message);
        else
        {
            FLARE_CORE_ERROR("Line {} Column {}: {}",
                error.Position.Line,
                error.Position.Column,
                error.Message);
        }
    }

    static std::optional<bool> BoolFromString(std::string_view string)
    {
        if (string == "true")
            return true;
        if (string == "false")
            return false;
        return {};
    }

    static void ParseShaderMetadata(const ShaderSourceParser& parser,
        ShaderFeatures& features,
        std::vector<ShaderError>& errors,
        const std::unordered_map<std::string, size_t>& propertyNameToIndex,
        ShaderProperties& properties)
    {
        const Block& rootBlock = parser.GetBlock(0);
        for (const auto& element : rootBlock.Elements)
        {
            if (element.Name.Value == "Culling")
            {
                auto culling = CullingModeFromString(element.Value.Value);
                if (culling)
                    features.Culling = *culling;
                else
                    errors.emplace_back(element.Value.Position, fmt::format("Unknown culling mode '{}'", element.Value.Value));
            }
            else if (element.Name.Value == "BlendMode")
            {
                auto blending = BlendModeFromString(element.Value.Value);
                if (blending)
                    features.Blending = *blending;
                else
                    errors.emplace_back(element.Value.Position, fmt::format("Unknown blending mode '{}'", element.Value.Value));
            }
            else if (element.Name.Value == "DepthTest")
            {
                std::optional<bool> value = BoolFromString(element.Value.Value);
                if (value)
                    features.DepthTesting = *value;
                else
                    errors.emplace_back(element.Value.Position, fmt::format("Expected bool, but got {}", element.Value.Value));
            }
            else if (element.Name.Value == "DepthWrite")
            {
                std::optional<bool> value = BoolFromString(element.Value.Value);
                if (value)
                    features.DepthWrite = *value;
                else
                    errors.emplace_back(element.Value.Position, fmt::format("Expected bool, but got {}", element.Value.Value));
            }
            else if (element.Name.Value == "DepthFunction")
            {
                auto depthFunction = DepthComparisonFunctionFromString(element.Value.Value);
                if (depthFunction)
                    features.DepthFunction = *depthFunction;
                else
                    errors.emplace_back(element.Value.Position, fmt::format("Unknown depth function '{}'", element.Value.Value));
            }
            else if (element.Name.Value == "Properties")
            {
                if (element.Type != BlockElementType::Block)
                {
                    errors.emplace_back(element.Value.Position, fmt::format("Expected an array of properties, instead of a value"));
                    continue;
                }

                const Block& block = parser.GetBlock(element.ChildBlockIndex);
                for (const BlockElement& property : block.Elements)
                {
                    auto it = propertyNameToIndex.find(std::string(property.Name.Value));
                    if (it == propertyNameToIndex.end())
                        continue;

                    ShaderProperty& shaderProperty = properties[it->second];
                    shaderProperty.Hidden = false;
                    shaderProperty.DisplayName = shaderProperty.Name;

                    if (property.Type != BlockElementType::Block)
                        continue;

                    const Block& childBlock = parser.GetBlock(property.ChildBlockIndex);
                    for (const BlockElement& element : childBlock.Elements)
                    {
                        if (element.Name.Value == "Type")
                        {
                            if (element.Type != BlockElementType::Value)
                            {
                                errors.emplace_back(element.Value.Position, "Expected a property type not a block");
                                continue;
                            }

                            std::string_view typeString = element.Value.Value;
                            if (typeString == "Color")
                            {
                                switch (shaderProperty.Type)
                                {
                                case ShaderDataType::Float3:
                                case ShaderDataType::Float4:
                                    shaderProperty.Flags |= SerializationValueFlags::Color;
                                    break;
                                default:
                                    errors.emplace_back(element.Value.Position, "Property type 'Color' is only compatible with vec3 or vec4");
                                    break;
                                }
                            }
                            else if (typeString == "HDR")
                            {
                                switch (shaderProperty.Type)
                                {
                                case ShaderDataType::Float3:
                                case ShaderDataType::Float4:
                                    shaderProperty.Flags |= SerializationValueFlags::HDRColor;
                                    break;
                                default:
                                    errors.emplace_back(element.Value.Position, "Property type 'HDR' is only compatible with vec3 or vec4");
                                    break;
                                }
                            }
                            else
                            {
                                errors.emplace_back(element.Value.Position, fmt::format("Unknown shader property type '{}'", element.Value.Value));
                                continue;
                            }
                        }

                        if (element.Name.Value == "DisplayName")
                        {
                            if (element.Type == BlockElementType::Value)
                                shaderProperty.DisplayName = element.Value.Value;
                            else
                                errors.emplace_back(element.Value.Position, "Expected a string value");
                        }
                    }
                }
            }
        }
    }

    static void Reflect(spirv_cross::Compiler& compiler,
        std::unordered_map<std::string, size_t>& propertyNameToIndex,
        ShaderProperties& properties)
    {
        const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();
      
        size_t lastPropertyOffset = 0;
        for (const auto& resource : resources.push_constant_buffers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            size_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

            uint32_t membersCount = (uint32_t)bufferType.member_types.size();

            for (uint32_t i = 0; i < membersCount; i++)
            {
                const std::string& memberName = compiler.get_member_name(resource.base_type_id, i);
                spirv_cross::TypeID memberTypeId = bufferType.member_types[i];
                size_t offset = compiler.type_struct_member_offset(bufferType, i);

                const auto& memberType = compiler.get_type(memberTypeId);

                std::optional<ShaderDataType> shaderDataType = {};
                uint32_t componentsCount = memberType.vecsize;

                bool error = false;

                switch (memberType.basetype)
                {
                case spirv_cross::SPIRType::BaseType::Int:
                    switch (componentsCount)
                    {
                    case 1:
                        shaderDataType = ShaderDataType::Int;
                        break;
                    case 2:
                        shaderDataType = ShaderDataType::Int2;
                        break;
                    case 3:
                        shaderDataType = ShaderDataType::Int2;
                        break;
                    case 4:
                        shaderDataType = ShaderDataType::Int2;
                        break;
                    default:
                        error = true;
                        FLARE_CORE_ERROR("Unsupported components count");
                    }

                    break;
                case spirv_cross::SPIRType::BaseType::Float:
                    switch (componentsCount)
                    {
                    case 1:
                        shaderDataType = ShaderDataType::Float;
                        break;
                    case 2:
                        shaderDataType = ShaderDataType::Float2;
                        break;
                    case 3:
                        shaderDataType = ShaderDataType::Float3;
                        break;
                    case 4:
                        if (memberType.columns == 4)
                            shaderDataType = ShaderDataType::Matrix4x4;
                        else
                            shaderDataType = ShaderDataType::Float4;
                        break;
                    default:
                        error = true;
                        FLARE_CORE_ERROR("Unsupported components count");
                    }

                    break;
                default:
                    error = true;
                    FLARE_CORE_ERROR("Unsupported shader data type");
                }

                if (error)
                    continue;

                if (shaderDataType.has_value())
                {
                    ShaderProperty& shaderProperty = properties.emplace_back();
                    shaderProperty.Location = UINT32_MAX;

                    if (resource.name.empty())
                        shaderProperty.Name = resource.name;
                    else
                        shaderProperty.Name = fmt::format("{}.{}", resource.name, memberName);

                    shaderProperty.Offset = lastPropertyOffset;
                    shaderProperty.Type = shaderDataType.value();
                    shaderProperty.Size = compiler.get_declared_struct_member_size(bufferType, i);
                    shaderProperty.Hidden = true;

                    propertyNameToIndex[shaderProperty.Name] = properties.size() - 1;
                }

                lastPropertyOffset += compiler.get_declared_struct_member_size(bufferType, i);
            }
        }

        uint32_t samplerIndex = 0;
        for (const auto& resource : resources.sampled_images)
        {
            const auto& samplerType = compiler.get_type(resource.type_id);
            uint32_t membersCount = (uint32_t)samplerType.member_types.size();
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

            ShaderDataType type = ShaderDataType::Sampler;
            size_t size = ShaderDataTypeSize(type);
            if (samplerType.array.size() > 0)
            {
                if (samplerType.array.size() == 1)
                {
                    type = ShaderDataType::SamplerArray;
                    size *= samplerType.array[0];
                }
                else
                {
                    FLARE_CORE_ERROR("Unsupported sampler array dimensions: {}", samplerType.array.size());
                    continue;
                }
            }

            ShaderProperty& shaderProperty = properties.emplace_back();
            shaderProperty.Location = binding;
            shaderProperty.Name = resource.name;
            shaderProperty.Offset = lastPropertyOffset;
            shaderProperty.Type = type;
            shaderProperty.Size = size;
            shaderProperty.Hidden = true;
            shaderProperty.SamplerIndex = samplerIndex;

            propertyNameToIndex[shaderProperty.Name] = properties.size() - 1;

            lastPropertyOffset += size;
            samplerIndex++;
        }
    }

    static void ExtractShaderOutputs(spirv_cross::Compiler& compiler, ShaderOutputs& outputs)
    {
        const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

        outputs.reserve(resources.stage_outputs.size());
        for (const auto& output : resources.stage_outputs)
        {
            uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
            outputs.push_back(location);
        }
    }
    
    bool ShaderCompiler::Compile(AssetHandle shaderHandle, bool forceRecompile)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

        const std::filesystem::path& shaderPath = AssetManager::GetAssetMetadata(shaderHandle)->Path;
        std::string sourceString = "";
        {
            std::ifstream file(shaderPath);
            if (!file.is_open())
            {
                FLARE_CORE_ERROR("Failed not read shader file {0}", shaderPath.string());
                return false;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            sourceString = buffer.str();
        }

        std::string pathString = shaderPath.string();
        std::vector<PreprocessedShaderProgram> programs;
        std::unordered_map<std::string, size_t> propertyNameToIndex;
        ShaderOutputs shaderOutputs;
        ShaderProperties shaderProperties;

        std::vector<ShaderError> errors;

        ShaderFeatures shaderFeatures;
        std::string_view source = sourceString;

        ShaderSourceParser parser(shaderPath, source, errors);
        parser.Parse();

        if (errors.size() == 0)
        {
            for (const auto& sourceBlock : parser.GetSourceBlocks())
            {
                PreprocessedShaderProgram& program = programs.emplace_back();
                program.Source = sourceBlock.Source;
                program.Stage = sourceBlock.Stage;
            }
        }
        else
        {
            FLARE_CORE_ERROR("Failed to compile shader '{}'", pathString);
            for (const ShaderError& error : errors)
                PrintError(error);

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

            CrossCompilationOptions options;
            options.LocationBase = 0;
            options.ProgramKind = shaderKind;

            if (shaderKind == shaderc_fragment_shader)
            {
                // NOTE: Uniform locations in pixel shaders start at 100,
                //       in order to avoid overlaps with vertex shader uniform locations

                // TODO: Find a better solution
                options.LocationBase = 100;
            }

            std::optional<std::vector<uint32_t>> compiledVulkanShader = {};

            if (!forceRecompile)
            {
                compiledVulkanShader = ShaderCacheManager::GetInstance()->FindCache(
                    shaderHandle,
                    ShaderTargetEnvironment::Vulkan,
                    program.Stage);
            }

            if (!compiledVulkanShader)
            {
                compiledVulkanShader = CompileVulkanGlslToSpirv(pathString, program.Source, shaderKind);
                if (!compiledVulkanShader)
                    return false;

                ShaderCacheManager::GetInstance()->SetCache(shaderHandle, ShaderTargetEnvironment::Vulkan, program.Stage, compiledVulkanShader.value());
            }

            try
            {
                spirv_cross::Compiler compiler(compiledVulkanShader.value());
                Reflect(compiler, propertyNameToIndex, shaderProperties);

                if (program.Stage == ShaderStageType::Pixel)
                    ExtractShaderOutputs(compiler, shaderOutputs);
            }
            catch (spirv_cross::CompilerError& e)
            {
                FLARE_CORE_ERROR("Shader reflection failed: {}", e.what());
            }

            if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
            {
				std::optional<std::vector<uint32_t>> compiledOpenGLShader = {};

				if (!forceRecompile)
				{
					compiledOpenGLShader = ShaderCacheManager::GetInstance()->FindCache(
						shaderHandle,
						ShaderTargetEnvironment::OpenGL,
						program.Stage);
				}

				if (!compiledOpenGLShader)
				{
					compiledOpenGLShader = CompileSpirvToGlsl(pathString, compiledVulkanShader.value(), options);
					if (!compiledOpenGLShader)
						return false;

					ShaderCacheManager::GetInstance()->SetCache(shaderHandle, ShaderTargetEnvironment::OpenGL, program.Stage, compiledOpenGLShader.value());
				}
			}
		}

        ParseShaderMetadata(parser, shaderFeatures, errors, propertyNameToIndex, shaderProperties);

        Ref<ShaderMetadata> metadata = CreateRef<ShaderMetadata>();
        metadata->Properties = std::move(shaderProperties);
        metadata->Features = shaderFeatures;
        metadata->Outputs = std::move(shaderOutputs);

        for (const auto& program : programs)
            metadata->Stages.push_back(program.Stage);

        ((EditorShaderCache*)ShaderCacheManager::GetInstance().get())->SetShaderEntry(shaderHandle, metadata);

        return true;
    }
}
