#include "VulkanShaderStorageBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanShaderStorageBuffer::VulkanShaderStorageBuffer(size_t size)
		: m_Buffer(
			GPUBufferUsage::Static,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VulkanBuffer::PipelineDependecy(
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT),
			size)
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

	void VulkanShaderStorageBuffer::SetData(const MemorySpan& data, size_t offset, Ref<CommandBuffer> commandBuffer)
	{
		m_Buffer.SetData(data, offset, commandBuffer);
	}

	void VulkanShaderStorageBuffer::Resize(size_t size)
	{
		m_Buffer.Resize(size);
	}

	void VulkanShaderStorageBuffer::SetDebugName(std::string_view name)
	{
		m_Buffer.SetDebugName(name);
	}

	const std::string& VulkanShaderStorageBuffer::GetDebugName() const
	{
		return m_Buffer.GetDebugName();
	}
}
