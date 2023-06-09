#include "OpenGLBuffer.h"

namespace Flare
{
	OpenGLVertexBuffer::OpenGLVertexBuffer()
	{
		glGenBuffers(1, &m_Id);
	}
	
	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLVertexBuffer::Bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	}
	
	void OpenGLVertexBuffer::SetData(const void* data, size_t size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
	}



	void OpenGLIndexBuffer::SetData(const void* indices, size_t size)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(uint32_t), indices, GL_DYNAMIC_DRAW);

		m_Size = size;
	}
	
	OpenGLIndexBuffer::OpenGLIndexBuffer()
	{
		glGenBuffers(1, &m_Id);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLIndexBuffer::Bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	}
}