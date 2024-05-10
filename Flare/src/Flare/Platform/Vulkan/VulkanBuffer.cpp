#include "VulkanBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "FlareCore/Profiler/Profiler.h"

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
		FLARE_PROFILE_FUNCTION();
		if (size  == 0)
			return;

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
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
			StagingBuffer stagingBuffer = FillStagingBuffer(MemorySpan::FromRawBytes(const_cast<void*>(data), size));

			commandBuffer->CopyBuffer(stagingBuffer.Buffer, m_Buffer, size, 0, offset);
			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);

			vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBuffer.Allocation.Handle);
			vkDestroyBuffer(VulkanContext::GetInstance().GetDevice(), stagingBuffer.Buffer, nullptr);
		}
	}

	void VulkanBuffer::SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (data.GetSize() == 0)
			return;

		if (m_Size == 0)
			m_Size = data.GetSize();

		FLARE_CORE_ASSERT(data.GetSize() <= m_Size);

		EnsureAllocated();

		StagingBuffer stagingBuffer = FillStagingBuffer(data);

		As<VulkanCommandBuffer>(commandBuffer)->CopyBuffer(stagingBuffer.Buffer, m_Buffer, data.GetSize(), 0, offset);

		VulkanContext::GetInstance().DeferDestroyStagingBuffer(std::move(stagingBuffer));
	}

	void VulkanBuffer::EnsureAllocated()
	{
		if (m_Buffer)
			return;
		
		Create();
	}

	void VulkanBuffer::Create()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Size > 0);
		FLARE_CORE_ASSERT(m_Buffer == VK_NULL_HANDLE);

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
			allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			break;
		case GPUBufferUsage::Dynamic:
			allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			break;
		}

		VK_CHECK_RESULT(vmaCreateBuffer(VulkanContext::GetInstance().GetMemoryAllocator(), &info, &allocation, &m_Buffer, &m_Allocation.Handle, &m_Allocation.Info));

		if (m_Usage == GPUBufferUsage::Dynamic)
		{
			VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle, &m_Mapped));
		}
	}

	StagingBuffer VulkanBuffer::FillStagingBuffer(MemorySpan data)
	{
		FLARE_PROFILE_FUNCTION();
		StagingBuffer stagingBuffer{};
		stagingBuffer.Allocation = VulkanContext::GetInstance().CreateStagingBuffer(data.GetSize(), stagingBuffer.Buffer);

		void* mapped = nullptr;
		VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBuffer.Allocation.Handle, &mapped));

		{
			FLARE_PROFILE_SCOPE("Copy");
			std::memcpy(mapped, data.GetBuffer(), data.GetSize());
		}

		vmaUnmapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBuffer.Allocation.Handle);

		return stagingBuffer;
	}
}
