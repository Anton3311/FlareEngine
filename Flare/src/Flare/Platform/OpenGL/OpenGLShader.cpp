#include "OpenGLShader.h"

#include "Flare.h"

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

    OpenGLShader::OpenGLShader(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();

            Compile(path, buffer.str());
        }
        else
            FLARE_CORE_ERROR("Could not read file {0}", path.string());
    }
    
    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_Id);
    }

    void Flare::OpenGLShader::Bind()
    {
        glUseProgram(m_Id);
    }

    const ShaderParameters& OpenGLShader::GetParameters() const
    {
        return m_Parameters;
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

    static std::filesystem::path SHADER_CACHE_LOCATION = "Cache/Shaders/";

    static std::filesystem::path GetCachePath(const std::filesystem::path& initialPath, std::string_view apiName, std::string_view stageName)
    {
        std::filesystem::path parent = std::filesystem::relative(initialPath.parent_path(), std::filesystem::current_path());
        std::filesystem::path cachePath = SHADER_CACHE_LOCATION / parent / fmt::format("{0}.{1}.chache.{2}", initialPath.filename().string(), apiName, stageName);

        return cachePath;
    }

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

        output.resize(size / sizeof(uint32_t));
        inputStream.read((char*)output.data(), size);

        return true;
    }

    static std::optional<SpirvData> CompileVulkanGlslToSpirv(const std::string& path, const std::string& source, shaderc_shader_kind programKind)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetGenerateDebugInfo();
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

    static void Reflect(const std::string& shaderPath, const SpirvData& shader, ShaderParameters& parameters)
    {
        try
        {
            spirv_cross::Compiler compiler(shader);
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();

            FLARE_CORE_INFO("Shader reflection '{0}'", shaderPath);
            FLARE_CORE_INFO("Uniform buffers:");

            for (const auto& resource : resources.uniform_buffers)
                PrintResourceInfo(compiler, resource);

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
                        parameters.emplace_back(memberName, dataType, offset);
                    else
                        parameters.emplace_back(fmt::format("{0}.{1}", resource.name, memberName), dataType, offset);
                }
            }
        }
        catch (spirv_cross::CompilerError& e)
        {
            FLARE_CORE_ERROR("Shader '{0}' reflection failed: {1}", shaderPath, e.what());
        }
    }

    void OpenGLShader::Compile(const std::filesystem::path& path, std::string_view source)
    {
        m_Id = glCreateProgram();
        auto programs = PreProcess(source);

        std::vector<uint32_t> shaderIds;
        shaderIds.reserve(programs.size());

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
            std::filesystem::path cachePath = GetCachePath(path, "vulkan", stageName);
            if (std::filesystem::exists(cachePath))
            {
                FLARE_CORE_ASSERT(ReadSpirvData(cachePath, compiledShaderData));
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

            Reflect(filePath, compiledShaderData, m_Parameters);

            cachePath = GetCachePath(path, "opengl", stageName);
            if (std::filesystem::exists(cachePath))
            {
                FLARE_CORE_ASSERT(ReadSpirvData(cachePath, compiledShaderData));
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

        m_UniformLocations.reserve(m_Parameters.size());
        for (const auto& param : m_Parameters)
        {
            int32_t location = glGetUniformLocation(m_Id, param.Name.c_str());
            FLARE_CORE_ASSERT(location != -1);

            m_UniformLocations.push_back(location);
        }
    }
}