#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

namespace Flare
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(size_t size, uint32_t binding)
		: m_Id(UINT32_MAX)
	{
		glCreateBuffers(1, &m_Id);
		glNamedBufferData(m_Id, size, nullptr, GL_DYNAMIC_DRAW);
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
}
