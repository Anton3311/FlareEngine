#pragma once

#include <Flare/Renderer/VertexArray.h>

namespace Flare
{
	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		~OpenGLVertexArray();
	public:
		virtual void Bind() override;
		virtual void Unbind() override;
		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;

		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexbuffer) override;
		virtual const Ref<IndexBuffer> GetIndexBuffer() const override;
	private:
		uint32_t m_Id;
		uint32_t m_VertexBufferIndex;
		Ref<IndexBuffer> m_IndexBuffer;
	};
}