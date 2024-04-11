#include "OpenGLVertexArray.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

namespace Flare
{
	OpenGLVertexArray::OpenGLVertexArray()
		: m_VertexBufferIndex(0)
	{
		glGenVertexArrays(1, &m_Id);
	}
	
	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_Id);
	}

	void OpenGLVertexArray::Bind() const
	{
		glBindVertexArray(m_Id);
	}

	void OpenGLVertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

	void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, std::optional<uint32_t> baseBinding)
	{
		glBindVertexArray(m_Id);
		vertexBuffer->Bind();

		const BufferLayout& layout = vertexBuffer->GetLayout();

		m_VertexBuffers.push_back(vertexBuffer);

		if (baseBinding)
			m_VertexBufferIndex = *baseBinding;

		for (const auto& element : layout.GetElements())
		{
			glEnableVertexAttribArray(m_VertexBufferIndex);
			switch (element.DataType)
			{
			case ShaderDataType::Int:
				glVertexAttribIPointer(m_VertexBufferIndex, 
					element.ComponentsCount, 
					GL_INT,
					layout.GetStride(), 
					(const void*)(size_t)element.Offset);
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
					(const void*)(size_t)element.Offset);
				break;
			case ShaderDataType::Matrix4x4:
				uint32_t offset = element.Offset;
				for (uint32_t i = 0; i < 4; i++)
				{
					glEnableVertexAttribArray(m_VertexBufferIndex);
					glVertexAttribPointer(m_VertexBufferIndex,
						4,
						GL_FLOAT,
						element.IsNormalized,
						layout.GetStride(),
						(const void*)(size_t)(offset + sizeof(glm::vec4) * i));

					if (i != 3)
						m_VertexBufferIndex++;
				}
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