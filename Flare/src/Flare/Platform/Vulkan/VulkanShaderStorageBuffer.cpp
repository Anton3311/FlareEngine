#include "VulkanShaderStorageBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanShaderStorageBuffer::VulkanShaderStorageBuffer(size_t size)
		: m_Buffer(GPUBufferUsage::Static, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size)
	{
		m_Buffer.EnsureAllocated();
	}

	size_t VulkanShaderStorageBuffer::GetSize() const
	{
		return m_Buffer.GetSize();
	}

	void VulkanShaderStorageBuffer::SetData(const MemorySpan& data)
	{
		m_Buffer.SetData(data.GetBuffer(), data.GetSize(), 0);
	}

	void VulkanShaderStorageBuffer::SetDebugName(std::string_view name)
	{
		m_DebugName = name;
		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_Buffer.GetBuffer(), m_DebugName.c_str());
	}

	const std::string& VulkanShaderStorageBuffer::GetDebugName() const
	{
		return m_DebugName;
	}
}
