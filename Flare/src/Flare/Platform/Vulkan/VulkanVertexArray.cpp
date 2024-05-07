#include "VulkanVertexArray.h"

namespace Flare
{
	const VertexArray::VertexBuffers& Flare::VulkanVertexArray::GetVertexBuffers() const
	{
		return m_VertexBuffers;
	}

	void Flare::VulkanVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexbuffer)
	{
		m_IndexBuffer = indexbuffer;
	}

	const Ref<IndexBuffer> Flare::VulkanVertexArray::GetIndexBuffer() const
	{
		return m_IndexBuffer;
	}

	void Flare::VulkanVertexArray::Bind() const
	{
	}

	void Flare::VulkanVertexArray::Unbind() const
	{
	}

	void Flare::VulkanVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, std::optional<uint32_t> baseBinding)
	{
		m_VertexBuffers.push_back(vertexBuffer);
	}
}
