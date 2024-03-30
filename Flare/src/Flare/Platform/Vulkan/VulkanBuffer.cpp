#include "VulkanBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanBuffer::VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage, size_t size)
		: m_Usage(usage), m_UsageFlags(bufferUsage), m_Size(size)
	{
		if (usage == GPUBufferUsage::Static)
		{
			m_UsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
	}

	VulkanBuffer::VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage)
		: m_Usage(usage), m_UsageFlags(bufferUsage), m_Size(0)
	{
	}

	VulkanBuffer::~VulkanBuffer()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());

		VkDevice device = VulkanContext::GetInstance().GetDevice();

		if (m_Mapped)
		{
			vmaUnmapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle);
			m_Mapped = nullptr;
		}

		vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle);
		vkDestroyBuffer(device, m_Buffer, nullptr);
	}

	void VulkanBuffer::SetData(const void* data, size_t size, size_t offset)
	{
		if (m_Size == 0)
			m_Size = size;

		FLARE_CORE_ASSERT(size <= m_Size);

		EnsureAllocated();

		if (m_Usage == GPUBufferUsage::Dynamic)
		{
			FLARE_CORE_ASSERT(m_Mapped);

			std::memcpy((uint8_t*)m_Mapped + offset, data, size);
		}
		else
		{
			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VulkanAllocation stagingBufferAllocation = VulkanContext::GetInstance().CreateStagingBuffer(size, stagingBuffer);

			void* mapped = nullptr;
			VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle, &mapped));

			std::memcpy(mapped, data, size);

			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
			commandBuffer->CopyBuffer(stagingBuffer, m_Buffer, size, 0, offset);
			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);

			vmaUnmapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle);
			vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle);
			vkDestroyBuffer(VulkanContext::GetInstance().GetDevice(), stagingBuffer, nullptr);
		}
	}

	void VulkanBuffer::EnsureAllocated()
	{
		if (m_Buffer)
			return;
		
		Create();
	}

	void VulkanBuffer::Create()
	{
		FLARE_CORE_ASSERT(m_Size > 0);

		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags = 0;
		info.pNext = nullptr;
		info.pQueueFamilyIndices = nullptr;
		info.queueFamilyIndexCount = 0;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.size = m_Size;
		info.usage = m_UsageFlags;

		VmaAllocationCreateInfo allocation{};

		switch (m_Usage)
		{
		case GPUBufferUsage::Static:
			allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			break;
		case GPUBufferUsage::Dynamic:
			allocation.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			break;
		}

		VK_CHECK_RESULT(vmaCreateBuffer(VulkanContext::GetInstance().GetMemoryAllocator(), &info, &allocation, &m_Buffer, &m_Allocation.Handle, &m_Allocation.Info));

		if (m_Usage == GPUBufferUsage::Dynamic)
		{
			VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle, &m_Mapped));
		}
	}
}
