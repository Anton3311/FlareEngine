#include "VulkanUniformBuffer.h"

namespace Flare
{
	VulkanUniformBuffer::VulkanUniformBuffer(size_t size)
		: m_Buffer(GPUBufferUsage::Dynamic, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size)
	{
		m_Buffer.EnsureAllocated();
	}

	void VulkanUniformBuffer::SetData(const void* data, size_t size, size_t offset)
	{
		m_Buffer.SetData(data, size, offset);
	}

	size_t VulkanUniformBuffer::GetSize() const
	{
		return m_Buffer.GetSize();
	}
}
