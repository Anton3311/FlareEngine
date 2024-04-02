#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

namespace Flare
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(size_t size, uint32_t binding)
		: m_Id(UINT32_MAX), m_Size(size), m_Binding(binding)
	{
		glCreateBuffers(1, &m_Id);
		glNamedBufferData(m_Id, m_Size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_Id);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLUniformBuffer::SetData(const void* data, size_t size, size_t offset)
	{
		glNamedBufferSubData(m_Id, offset, size, data);
	}

	void OpenGLUniformBuffer::Bind() const
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_Id);
	}

	size_t OpenGLUniformBuffer::GetSize() const
	{
		return m_Size;
	}
}
