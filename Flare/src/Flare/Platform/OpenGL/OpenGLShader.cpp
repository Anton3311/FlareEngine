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

        m_NameToIndex.clear();
        m_IsValid = true;
        m_Id = 0;

        m_Metadata = ShaderCacheManager::GetInstance()->FindShaderMetadata(Handle);
        m_IsValid = m_Metadata != nullptr;

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

                FLARE_CORE_ERROR("Failed to link shader: {}", infoLog.data());
                m_IsValid = false;
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
            const auto& properties = m_Metadata->Properties;
            for (size_t i = 0; i < properties.size(); i++)
                m_NameToIndex.emplace(properties[i].Name, (uint32_t)i);

            m_UniformLocations.reserve(properties.size());
            for (const auto& property : properties)
            {
                int32_t location = glGetUniformLocation(m_Id, property.Name.c_str());
                m_UniformLocations.push_back(location);
            }
        }
    }

    bool OpenGLShader::IsLoaded() const
    {
        return m_IsValid;
    }

    void Flare::OpenGLShader::Bind()
    {
        if (m_IsValid)
			glUseProgram(m_Id);
    }

    Ref<const ShaderMetadata> OpenGLShader::GetMetadata() const
    {
        return m_Metadata;
    }

    const ShaderProperties& OpenGLShader::GetProperties() const
    {
        return m_Metadata->Properties;
    }

    const ShaderOutputs& OpenGLShader::GetOutputs() const
    {
        return m_Metadata->Outputs;
    }

    ShaderFeatures OpenGLShader::GetFeatures() const
    {
        return m_Metadata->Features;
    }

    std::optional<uint32_t> OpenGLShader::GetPropertyIndex(std::string_view name) const
    {
        auto it = m_NameToIndex.find(name);
        if (it == m_NameToIndex.end())
            return {};
        return it->second;
    }
}