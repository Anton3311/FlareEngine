#pragma once

#include "Flare/Renderer/Buffer.h"

#include <vector>

namespace Flare
{
	class FLARE_API VertexArray
	{
	public:
		using VertexBuffers = std::vector<Ref<VertexBuffer>>;

		virtual const VertexBuffers& GetVertexBuffers() const = 0;

		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexbuffer) = 0;
		virtual const Ref<IndexBuffer> GetIndexBuffer() const = 0;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void AddInstanceBuffer(const Ref<VertexBuffer>& instanceBuffer) = 0;
	public:
		static Ref<VertexArray> Create();
	};
}