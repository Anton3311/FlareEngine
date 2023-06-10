#include "OpenGLVertexArray.h"

#include <glad/glad.h>

namespace Flare
{
	OpenGLVertexArray::OpenGLVertexArray()
		: m_VertexBufferIndex(0)
	{
		glGenVertexArrays(1, &m_Id);

		GLenum error = glGetError();
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	}
	
	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_Id);
	}

	void OpenGLVertexArray::Bind()
	{
		glBindVertexArray(m_Id);
	}

	void OpenGLVertexArray::Unbind()
	{
		glBindVertexArray(0);
	}
	
	void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		glBindVertexArray(m_Id);
		vertexBuffer->Bind();

		const BufferLayout& layout = vertexBuffer->GetLayout();

		m_VertexBuffers.push_back(vertexBuffer);

		for (const auto& element : layout.GetElements())
		{
			glEnableVertexAttribArray(m_VertexBufferIndex);
			switch (element.DataType)
			{
			case ShaderDataType::Int:
				glVertexAttribPointer(m_VertexBufferIndex, 
					element.ComponentsCount, 
					GL_INT, 
					element.IsNormalized, 
					layout.GetStride(), 
					(const void*)element.Offset);
				break;
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
				glVertexAttribPointer(m_VertexBufferIndex,
					element.ComponentsCount,
					GL_FLOAT,
					element.IsNormalized,
					layout.GetStride(),
					(const void*)element.Offset);
				break;
			}

			m_VertexBufferIndex++;
		}
	}

	void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexbuffer)
	{
		m_IndexBuffer = indexbuffer;

		glBindVertexArray(m_Id);
		m_IndexBuffer->Bind();
	}
	
	const Ref<IndexBuffer> OpenGLVertexArray::GetIndexBuffer() const
	{
		return m_IndexBuffer;
	}
}