#pragma once

#include "Flare/Renderer/VertexArray.h"

namespace Flare
{
	class VulkanVertexArray : public VertexArray
	{
	public:
		const VertexBuffers& GetVertexBuffers() const override;
		void SetIndexBuffer(const Ref<IndexBuffer>& indexbuffer) override;
		const Ref<IndexBuffer> GetIndexBuffer() const override;
		void Bind() const override;
		void Unbind() const override;
		void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, std::optional<uint32_t> baseBinding) override;
	private:
		std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;
	};
}
