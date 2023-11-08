#include "OpenGLShader.h"

#include "Flare.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/ShaderCacheManager.h"

#include <string>
#include <string_view>
#include <sstream>
#include <fstream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace Flare
{
    static shaderc_shader_kind GLShaderTypeToShaderC(uint32_t type)
    {
        switch (type)
        {
        case GL_VERTEX_SHADER:
            return shaderc_vertex_shader;
        case GL_FRAGMENT_SHADER:
            return shaderc_fragment_shader;
        }

        return (shaderc_shader_kind)0;
    }

    static void PrintResourceInfo(spirv_cross::Compiler& compiler, const spirv_cross::Resource& resource)
    {
        const auto& bufferType = compiler.get_type(resource.base_type_id);
        size_t bufferSize = compiler.get_declared_struct_size(bufferType);
        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

        uint32_t membersCount = (uint32_t)bufferType.member_types.size();

        FLARE_CORE_INFO("\tName = {0} Size = {1} Binding = {2} MembersCount = {3}", resource.name, bufferSize, binding, membersCount);

        for (uint32_t i = 0; i < membersCount; i++)
            FLARE_CORE_INFO("\t\t{0}: {1} {2}", i, compiler.get_name(bufferType.member_types[i]), compiler.get_member_name(resource.base_type_id, i));
    }

    static void Reflect(spirv_cross::Compiler& compiler, ShaderProperties& properties)
    {
        const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

        FLARE_CORE_INFO("Uniform buffers:");

        for (const auto& resource : resources.uniform_buffers)
            PrintResourceInfo(compiler, resource);

        FLARE_CORE_INFO("Samplers:");
        for (const auto& resource : resources.sampled_images)
        {
            const auto& samplerType = compiler.get_type(resource.type_id);
            uint32_t membersCount = (uint32_t)samplerType.member_types.size();

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
                    FLARE_CORE_ERROR("Unsupported sampler array dimensions: {0}", samplerType.array.size());
                    continue;
                }
            }

            properties.emplace_back(resource.name, type, size, 0);
        }

        FLARE_CORE_INFO("Push constant buffers:");
        for (const auto& resource : resources.push_constant_buffers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            size_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

            uint32_t membersCount = (uint32_t)bufferType.member_types.size();

            FLARE_CORE_INFO("\tName = {0} Size = {1} Binding = {2} MembersCount = {3}", resource.name, bufferSize, binding, membersCount);

            for (uint32_t i = 0; i < membersCount; i++)
            {
                const std::string& memberName = compiler.get_member_name(resource.base_type_id, i);
                spirv_cross::TypeID memberTypeId = bufferType.member_types[i];
                size_t offset = compiler.type_struct_member_offset(bufferType, i);

                const auto& memberType = compiler.get_type(memberTypeId);

                ShaderDataType dataType = ShaderDataType::Int;
                uint32_t componentsCount = memberType.vecsize;

                bool error = false;

                switch (memberType.basetype)
                {
                case spirv_cross::SPIRType::BaseType::Int:
                    switch (componentsCount)
                    {
                    case 1:
                        dataType = ShaderDataType::Int;
                        break;
                    case 2:
                        dataType = ShaderDataType::Int2;
                        break;
                    case 3:
                        dataType = ShaderDataType::Int3;
                        break;
                    case 4:
                        dataType = ShaderDataType::Int4;
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
                        dataType = ShaderDataType::Float;
                        break;
                    case 2:
                        dataType = ShaderDataType::Float2;
                        break;
                    case 3:
                        dataType = ShaderDataType::Float3;
                        break;
                    case 4:
                        if (memberType.columns == 4)
                            dataType = ShaderDataType::Matrix4x4;
                        else
                            dataType = ShaderDataType::Float4;
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

                if (resource.name.empty())
                    properties.emplace_back(memberName, dataType, offset);
                else
                    properties.emplace_back(fmt::format("{0}.{1}", resource.name, memberName), dataType, offset);
            }
        }
    }

    static void ExtractShaderOutputs(spirv_cross::Compiler& compiler, ShaderOutputs& outputs)
    {
        const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

        FLARE_CORE_INFO("Pixel stage outputs");

        outputs.reserve(resources.stage_outputs.size());
        for (const auto& output : resources.stage_outputs)
        {
            uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
            FLARE_CORE_INFO("\tName = {} Location = {}", output.name, location);

            outputs.push_back(location);
        }
    }

    OpenGLShader::OpenGLShader()
        : m_Id(0)
    {
    }

    OpenGLShader::OpenGLShader(const std::filesystem::path& path, const std::filesystem::path& cacheDirectory)
        : m_Id(0)
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();

            Compile(path, cacheDirectory, buffer.str());
        }
        else
            FLARE_CORE_ERROR("Could not read file {0}", path.string());
    }
    
    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_Id);
    }

    void OpenGLShader::Load()
    {
        m_Id = glCreateProgram();

        struct Program
        {
            uint32_t Id;
            ShaderStageType Stage;
        };

        Program programs[2];
        programs[0].Id = 0;
        programs[0].Stage = ShaderStageType::Vertex;
        programs[1].Id = 0;
        programs[1].Stage = ShaderStageType::Pixel;

        for (Program& program : programs)
        {
            std::string_view stageName = "";
            switch (program.Stage)
            {
            case ShaderStageType::Vertex:
                stageName = "vertex";
                break;
            case ShaderStageType::Pixel:
                stageName = "pixel";
                break;
            }

            auto vulkanShaderCache = ShaderCacheManager::GetInstance()->FindCache(Handle, ShaderTargetEnvironment::Vulkan, program.Stage);
            FLARE_CORE_ASSERT(vulkanShaderCache);

            try
            {
                spirv_cross::Compiler compiler(vulkanShaderCache.value());
                Reflect(compiler, m_Properties);

                if (program.Stage == ShaderStageType::Pixel)
                    ExtractShaderOutputs(compiler, m_Outputs);
            }
            catch (spirv_cross::CompilerError& e)
            {
                FLARE_CORE_ERROR("Shader '{0}' reflection failed: {1}",
                    AssetManager::GetAssetMetadata(Handle)->Path.string(),
                    e.what());
            }

            auto cachedShader = ShaderCacheManager::GetInstance()->FindCache(Handle, ShaderTargetEnvironment::OpenGL, program.Stage);
            FLARE_CORE_ASSERT(cachedShader, "Failed to find shader cache");

            uint32_t shaderType = 0;
            switch (program.Stage)
            {
            case ShaderStageType::Vertex:
                shaderType = GL_VERTEX_SHADER;
                break;
            case ShaderStageType::Pixel:
                shaderType = GL_FRAGMENT_SHADER;
                break;
            }

            GLuint shaderId = glCreateShader(shaderType);
            glShaderBinary(1, &shaderId,
                GL_SHADER_BINARY_FORMAT_SPIR_V,
                cachedShader.value().data(),
                (int32_t)(cachedShader.value().size() * sizeof(uint32_t)));

            glSpecializeShader(shaderId, "main", 0, nullptr, nullptr);
            glAttachShader(m_Id, shaderId);

            program.Id = shaderId;
        }

        glLinkProgram(m_Id);
        GLint isLinked = 0;
        glGetProgramiv(m_Id, GL_LINK_STATUS, (int*)&isLinked);

        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength + 1);
            glGetProgramInfoLog(m_Id, maxLength, &maxLength, &infoLog[0]);
            glDeleteProgram(m_Id);

            FLARE_CORE_ERROR("Failed to link shader: {0}", infoLog.data());

            for (const auto& program : programs)
                glDeleteShader(program.Id);
            return;
        }

        for (const auto& program : programs)
            glDetachShader(m_Id, program.Id);

        for (size_t i = 0; i < m_Properties.size(); i++)
            m_NameToIndex.emplace(m_Properties[i].Name, (uint32_t)i);

        m_UniformLocations.reserve(m_Properties.size());
        for (const auto& param : m_Properties)
        {
            int32_t location = glGetUniformLocation(m_Id, param.Name.c_str());
            FLARE_CORE_ASSERT(location != -1);

            m_UniformLocations.push_back(location);
        }
    }

    void Flare::OpenGLShader::Bind()
    {
        glUseProgram(m_Id);
    }

    const ShaderProperties& OpenGLShader::GetProperties() const
    {
        return m_Properties;
    }

    const ShaderOutputs& OpenGLShader::GetOutputs() const
    {
        return m_Outputs;
    }

    std::optional<uint32_t> OpenGLShader::GetPropertyIndex(std::string_view name) const
    {
        auto it = m_NameToIndex.find(name);
        if (it == m_NameToIndex.end())
            return {};
        return it->second;
    }

    void OpenGLShader::SetInt(const std::string& name, int value)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniform1i(location, value);
    }

    void OpenGLShader::SetFloat(const std::string& name, float value)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniform1f(location, value);
    }

    void OpenGLShader::SetFloat2(const std::string& name, glm::vec2 value)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniform2f(location, value.x, value.y);
    }

    void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniform3f(location, value.x, value.y, value.z);
    }

    void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::SetIntArray(const std::string& name, const int* values, uint32_t count)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniform1iv(location, count, values);
    }

    void OpenGLShader::SetMatrix4(const std::string& name, const glm::mat4& matrix)
    {
        int32_t location = glGetUniformLocation(m_Id, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void OpenGLShader::SetInt(uint32_t index, int32_t value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform1i(m_UniformLocations[index], value);
    }

    void OpenGLShader::SetInt2(uint32_t index, glm::ivec2 value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform2i(m_UniformLocations[index], value.x, value.y);
    }

    void OpenGLShader::SetInt3(uint32_t index, const glm::ivec3& value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform3i(m_UniformLocations[index], value.x, value.y, value.z);
    }

    void OpenGLShader::SetInt4(uint32_t index, const glm::ivec4& value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform4i(m_UniformLocations[index], value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::SetFloat(uint32_t index, float value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform1f(m_UniformLocations[index], value);
    }

    void OpenGLShader::SetFloat2(uint32_t index, glm::vec2 value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform2f(m_UniformLocations[index], value.x, value.y);
    }

    void OpenGLShader::SetFloat3(uint32_t index, const glm::vec3& value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform3f(m_UniformLocations[index], value.x, value.y, value.z);
    }

    void OpenGLShader::SetFloat4(uint32_t index, const glm::vec4& value)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform4f(m_UniformLocations[index], value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::SetIntArray(uint32_t index, const int* values, uint32_t count)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniform1iv(m_UniformLocations[index], count, values);
    }

    void OpenGLShader::SetMatrix4(uint32_t index, const glm::mat4& matrix)
    {
        FLARE_CORE_ASSERT((size_t)index < m_Properties.size());
        glUniformMatrix4fv(m_UniformLocations[index], 1, GL_FALSE, glm::value_ptr(matrix));
    }

    std::vector<OpenGLShader::ShaderProgram> OpenGLShader::PreProcess(std::string_view source)
    {
        std::vector<ShaderProgram> programs;

        std::string_view typeToken = "#type";

        size_t position = source.find_first_of(typeToken);
        while (position != std::string_view::npos)
        {
            size_t endOfLine = source.find_first_of("\r\n", position);
            size_t typeStart = position + typeToken.size() + 1;
            size_t nextLineStart = source.find_first_of("\r\n", endOfLine);

            std::string_view type = source.substr(typeStart, endOfLine - typeStart);

            GLenum shaderType;
            if (type == "vertex")
                shaderType = GL_VERTEX_SHADER;
            else if (type == "fragment")
                shaderType = GL_FRAGMENT_SHADER;
            else
                FLARE_CORE_ERROR("Invalid shader type {0}", type);

            position = source.find(typeToken, nextLineStart);

            std::string_view programSource;
            if (position == std::string_view::npos)
                programSource = source.substr(nextLineStart, source.size() - nextLineStart);
            else
                programSource = source.substr(nextLineStart, position - nextLineStart);

            programs.emplace_back(std::string(programSource.data(), programSource.size()), shaderType);
        }

        return programs;
    }

    using SpirvData = std::vector<uint32_t>;

    static void WriteSpirvData(const std::filesystem::path& path, const SpirvData& data)
    {
        std::ofstream output(path, std::ios::out | std::ios::binary);
        output.write((const char*)data.data(), data.size() * sizeof(uint32_t));
        output.close();
    }

    static bool ReadSpirvData(const std::filesystem::path& path, SpirvData& output)
    {
        std::ifstream inputStream(path, std::ios::in | std::ios::binary);

        if (!inputStream.is_open())
            return false;

        inputStream.seekg(0, std::ios::end);
        size_t size = inputStream.tellg();
        inputStream.seekg(0, std::ios::beg);

        FLARE_CORE_INFO("Reading SPIRV: Size = {}", size);

        output.resize(size / sizeof(uint32_t));
        inputStream.read((char*)output.data(), size);

        return true;
    }

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

    static std::optional<SpirvData> CompileVulkanGlslToSpirv(const std::string& path, const std::string& source, shaderc_shader_kind programKind)
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

    static std::optional<SpirvData> CompileSpirvToGlsl(const std::string& path, const SpirvData& spirvData, shaderc_shader_kind programKind)
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

    void OpenGLShader::Compile(const std::filesystem::path& path, const std::filesystem::path& cacheDirectory, std::string_view source)
    {
        m_Id = glCreateProgram();
        auto programs = PreProcess(source);

        std::vector<uint32_t> shaderIds;
        shaderIds.reserve(programs.size());

        if (!std::filesystem::exists(cacheDirectory))
            std::filesystem::create_directories(cacheDirectory);

        std::string filePath = path.generic_string();
        for (auto& program : programs)
        {
            std::string_view stageName = "";
            switch (program.Type)
            {
            case GL_VERTEX_SHADER:
                stageName = "vertex";
                break;
            case GL_FRAGMENT_SHADER:
                stageName = "pixel";
                break;
            }

            SpirvData compiledShaderData;
            std::filesystem::path cachePath = cacheDirectory / Shader::GetCacheFileName(path.filename().string(), "vulkan", stageName);
            if (std::filesystem::exists(cachePath))
            {
                bool result = ReadSpirvData(cachePath, compiledShaderData);
                FLARE_CORE_ASSERT(result);
            }
            else
            {
                std::filesystem::create_directories(cachePath.parent_path());

                std::optional<SpirvData> spirvData = CompileVulkanGlslToSpirv(filePath, program.Source, GLShaderTypeToShaderC(program.Type));
                if (!spirvData.has_value())
                    continue;

                compiledShaderData = std::move(spirvData.value());
                WriteSpirvData(cachePath, compiledShaderData);
            }

            FLARE_CORE_INFO("Shader reflection '{0}'", filePath);

            try
            {
                spirv_cross::Compiler compiler(compiledShaderData);
                Reflect(compiler, m_Properties);

                if (program.Type == GL_FRAGMENT_SHADER)
                    ExtractShaderOutputs(compiler, m_Outputs);
            }
            catch (spirv_cross::CompilerError& e)
            {
                FLARE_CORE_ERROR("Shader '{0}' reflection failed: {1}", filePath, e.what());
            }

            for (const auto& p : m_Properties)
                FLARE_CORE_INFO("\t\tName = {0} Size = {1} Offset = {2}", p.Name, p.Size, p.Offset);

            cachePath = cacheDirectory / Shader::GetCacheFileName(path.filename().string(), "opengl", stageName);
            if (std::filesystem::exists(cachePath))
            {
                bool result = ReadSpirvData(cachePath, compiledShaderData);
                FLARE_CORE_ASSERT(result);
            }
            else
            {
                std::filesystem::create_directories(cachePath.parent_path());

                std::optional<SpirvData> compiledShader = CompileSpirvToGlsl(filePath, compiledShaderData, GLShaderTypeToShaderC(program.Type));
                if (!compiledShader.has_value())
                    continue;

                compiledShaderData = std::move(compiledShader.value());
                WriteSpirvData(cachePath, compiledShaderData);
            }

            GLuint shader = glCreateShader(program.Type);
            glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, compiledShaderData.data(), (int32_t)(compiledShaderData.size() * sizeof(uint32_t)));
            glSpecializeShader(shader, "main", 0, nullptr, nullptr);
            glAttachShader(m_Id, shader);

            shaderIds.push_back(shader);
        }

        glLinkProgram(m_Id);
        GLint isLinked = 0;
        glGetProgramiv(m_Id, GL_LINK_STATUS, (int*)&isLinked);

        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength + 1);
            glGetProgramInfoLog(m_Id, maxLength, &maxLength, &infoLog[0]);
            glDeleteProgram(m_Id);

            FLARE_CORE_ERROR("Failed to link shader: {0}", infoLog.data());

            for (auto id : shaderIds)
                glDeleteShader(id);
            return;
        }

        for (auto id : shaderIds)
            glDetachShader(m_Id, id);

        for (size_t i = 0; i < m_Properties.size(); i++)
            m_NameToIndex.emplace(m_Properties[i].Name, (uint32_t)i);

        m_UniformLocations.reserve(m_Properties.size());
        for (const auto& param : m_Properties)
        {
            int32_t location = glGetUniformLocation(m_Id, param.Name.c_str());
            FLARE_CORE_ASSERT(location != -1);

            m_UniformLocations.push_back(location);
        }
    }
}