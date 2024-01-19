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

        size_t lastPropertyOffset = 0;
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

                lastPropertyOffset = offset + compiler.get_declared_struct_member_size(bufferType, i);
            }
        }

        FLARE_CORE_INFO("Samplers:");
        for (const auto& resource : resources.sampled_images)
        {
            const auto& samplerType = compiler.get_type(resource.type_id);
            uint32_t membersCount = (uint32_t)samplerType.member_types.size();

            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

            ShaderDataType type = ShaderDataType::Sampler;

            // size = sizeof(AssetHandle) because for non-array samplers an asset handle to a texture is stored.
            size_t size = sizeof(AssetHandle);
            if (samplerType.array.size() > 0)
            {
                if (samplerType.array.size() == 1)
                {
                    type = ShaderDataType::SamplerArray;
                    size = ShaderDataTypeSize(ShaderDataType::Sampler) * samplerType.array[0];
                }
                else
                {
                    FLARE_CORE_ERROR("Unsupported sampler array dimensions: {0}", samplerType.array.size());
                    continue;
                }
            }

            auto& property = properties.emplace_back(resource.name, type, size, lastPropertyOffset);
            property.Location = binding;
            lastPropertyOffset += sizeof(AssetHandle);
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
        : m_Id(0), m_IsValid(false)
    {
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_Id);
    }

    void OpenGLShader::Load()
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(Handle));

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

        bool hasValidCache = true;

        for (const Program& program : programs)
        {
            if (!ShaderCacheManager::GetInstance()->HasCache(Handle, ShaderTargetEnvironment::OpenGL, program.Stage)
                || !ShaderCacheManager::GetInstance()->HasCache(Handle, ShaderTargetEnvironment::Vulkan, program.Stage))
            {
                hasValidCache = false;
                break;
            }
        }

        if (!hasValidCache)
            return;

        if (m_Id != 0)
            glDeleteProgram(m_Id);

        m_Properties.clear();
        m_NameToIndex.clear();
        m_Features = ShaderFeatures();
        m_Outputs.clear();
        m_IsValid = true;
        m_Id = 0;

        auto features = ShaderCacheManager::GetInstance()->FindShaderFeatures(Handle);
        m_IsValid = features.has_value();
        if (features)
        {
            m_Features = features.value();
        }

        if (!m_IsValid)
            return;

        m_Id = glCreateProgram();

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
            if (!vulkanShaderCache.has_value())
            {
                FLARE_CORE_ERROR("Failed to find cached Vulkan shader code");

                m_IsValid = false;
                break;
            }

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
            if (!cachedShader.has_value())
            {
                FLARE_CORE_ERROR("Failed to find cached OpenGL shader code");

                m_IsValid = false;
                break;
            }

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

        if (m_IsValid)
        {
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

                FLARE_CORE_ERROR("Failed to link shader: {}", infoLog.data());
                return;
            }
        }

        for (const auto& program : programs)
        {
            if (program.Id != 0)
            {
                glDetachShader(m_Id, program.Id);
                glDeleteShader(program.Id);
            }
        }

        if (m_IsValid)
        {
            for (size_t i = 0; i < m_Properties.size(); i++)
                m_NameToIndex.emplace(m_Properties[i].Name, (uint32_t)i);

            m_UniformLocations.reserve(m_Properties.size());
            for (const auto& param : m_Properties)
            {
                int32_t location = glGetUniformLocation(m_Id, param.Name.c_str());
                //FLARE_CORE_ASSERT(location != -1);

                m_UniformLocations.push_back(location);
            }
        }
    }

    bool OpenGLShader::IsLoaded()
    {
        return m_IsValid;
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

    ShaderFeatures OpenGLShader::GetFeatures() const
    {
        return m_Features;
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
}