#include "OpenGLBuffer.h"

namespace Flare
{
	OpenGLVertexBuffer::OpenGLVertexBuffer(size_t size)
		: m_BufferSize(size)
	{
		glCreateBuffers(1, &m_Id);
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(size_t size, const void* data)
		: m_BufferSize(size)
	{
		glCreateBuffers(1, &m_Id);
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}
	
	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLVertexBuffer::Bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	}
	
	void OpenGLVertexBuffer::SetData(const void* data, size_t size, size_t offset)
	{
		FLARE_CORE_ASSERT(offset + size <= m_BufferSize);

		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
	}

	void OpenGLVertexBuffer::SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer)
	{
		SetData(data.GetBuffer(), data.GetSize(), offset);
	}



	OpenGLIndexBuffer::OpenGLIndexBuffer(IndexFormat format, size_t size)
	{
		size_t indexSize = 0;
		switch (format)
		{
		case IndexFormat::UInt16:
			indexSize = sizeof(uint16_t);
			break;
		case IndexFormat::UInt32:
			indexSize = sizeof(uint32_t);
			break;
		}

		m_Format = format;
		m_SizeInBytes = size * indexSize;

		glGenBuffers(1, &m_Id);
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		glBufferData(GL_ARRAY_BUFFER, m_SizeInBytes, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(IndexFormat format, const MemorySpan& indices)
	{
		glGenBuffers(1, &m_Id);
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		glBufferData(GL_ARRAY_BUFFER, indices.GetSize(), indices.GetBuffer(), GL_STATIC_DRAW);

		m_SizeInBytes = indices.GetSize();
		m_Format = format;
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLIndexBuffer::Bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	}

	void OpenGLIndexBuffer::SetData(const MemorySpan& indices, size_t offset)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, indices.GetSize(), indices.GetBuffer());
	}

	void OpenGLIndexBuffer::SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer)
	{
		SetData(data, offset);
	}

	size_t OpenGLIndexBuffer::GetCount() const
	{
		switch (m_Format)
		{
		case IndexBuffer::IndexFormat::UInt16:
			return m_SizeInBytes / sizeof(uint16_t);
		case IndexBuffer::IndexFormat::UInt32:
			return m_SizeInBytes / sizeof(uint32_t);
		}

		FLARE_CORE_ASSERT(false);
		return 0;
	}

	IndexBuffer::IndexFormat OpenGLIndexBuffer::GetIndexFormat() const
	{
		return m_Format;
	}
}