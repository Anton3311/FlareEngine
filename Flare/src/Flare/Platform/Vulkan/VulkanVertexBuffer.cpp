#include "VulkanVertexBuffer.h"

namespace Flare
{
	VulkanVertexBuffer::VulkanVertexBuffer(size_t size)
		: m_Buffer(GPUBufferUsage::Dynamic, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size)
	{
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const void* data, size_t size)
		: m_Buffer(GPUBufferUsage::Static, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
	{
		m_Buffer.SetData(data, size, 0);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
	}
	const BufferLayout& VulkanVertexBuffer::GetLayout() const
	{
		return m_Layout;
	}

	void VulkanVertexBuffer::SetLayout(const BufferLayout& layout)
	{
		m_Layout = layout;
	}

	void VulkanVertexBuffer::Bind()
	{
	}

	void VulkanVertexBuffer::SetData(const void* data, size_t size, size_t offset)
	{
		m_Buffer.SetData(data, size, offset);
	}
}
