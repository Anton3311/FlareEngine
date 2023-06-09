#pragma once

#include <Flare/Renderer/Buffer.h>

#include <vector>

namespace Flare
{
	class VertexArray
	{
	public:
		using VertexBuffers = std::vector<Ref<VertexBuffer>>;

		const VertexBuffers& GetVertexBuffers() const { return m_VertexBuffers; }

		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexbuffer) = 0;
		virtual const Ref<IndexBuffer> GetIndexBuffer() const = 0;

		virtual void Bind() = 0;
		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
	public:
		static Ref<VertexArray> Create();
	protected:
		VertexBuffers m_VertexBuffers;
	};
}