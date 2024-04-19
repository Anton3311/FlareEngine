#include "OpenGLShaderStorageBuffer.h"

#include <glad/glad.h>

namespace Flare
{
    OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(uint32_t binding)
        : m_Id(0), m_Binding(binding), m_Size(0)
    {
        glCreateBuffers(1, &m_Id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_Id);
    }

    OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
    {
        glDeleteBuffers(1, &m_Id);
    }

    size_t OpenGLShaderStorageBuffer::GetSize() const
    {
        return m_Size;
    }

    void OpenGLShaderStorageBuffer::SetData(const MemorySpan& data)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, data.GetSize(), data.GetBuffer(), GL_DYNAMIC_DRAW);
    }

    void OpenGLShaderStorageBuffer::SetDebugName(std::string_view name)
    {
        // TODO: Set the debug name of an OpenGL object
        m_DebugName = name;
    }

    const std::string& OpenGLShaderStorageBuffer::GetDebugName() const
    {
        return m_DebugName;
    }
}
