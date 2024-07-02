#include "VulkanStagingBufferPool.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanStagingBufferPool::VulkanStagingBufferPool(size_t maxEntrySize, size_t maxReservedPoolEntries)
		: m_MaxPoolEntrySize(maxEntrySize), m_MaxReservedPoolEntries(maxReservedPoolEntries)
	{
	}

	VulkanStagingBuffer VulkanStagingBufferPool::AllocateStagingBuffer(size_t size)
	{
		FLARE_PROFILE_FUNCTION();
		if (size > m_MaxPoolEntrySize)
		{
			SeparateBufferAllocation& separateAllocation = m_SeparateAllocations.emplace_back();
			separateAllocation.Size = size;

			AllocateBuffer(separateAllocation.Size, separateAllocation.Buffer, separateAllocation.Allocation);

			VK_CHECK_RESULT(vmaMapMemory(
				VulkanContext::GetInstance().GetMemoryAllocator(),
				separateAllocation.Allocation.Handle,
				&separateAllocation.Mapped));

			VulkanStagingBuffer stagingBuffer{};
			stagingBuffer.Offset = 0;
			stagingBuffer.Size = separateAllocation.Size;
			stagingBuffer.Buffer = separateAllocation.Buffer;
			stagingBuffer.Mapped = separateAllocation.Mapped;

			return stagingBuffer;
		}

		auto it = std::find_if(m_PoolEntries.begin(), m_PoolEntries.end(), [size](const PoolEntry& entry) -> bool
			{
				return size <= entry.Size - entry.BytesAllocated;
			});

		if (it == m_PoolEntries.end())
		{
			CreateBufferEntry();
			it = m_PoolEntries.end() - 1;
		}

		FLARE_CORE_ASSERT(it->Mapped);

		VulkanStagingBuffer stagingBuffer{};
		stagingBuffer.Buffer = it->Buffer;
		stagingBuffer.Offset = it->BytesAllocated;
		stagingBuffer.Size = size;
		stagingBuffer.Mapped = (uint8_t*)it->Mapped + stagingBuffer.Offset;

		it->BytesAllocated += size;

		// TODO: alignment?

		return stagingBuffer;
	}

	void VulkanStagingBufferPool::ReleaseStagingBuffer(VulkanStagingBuffer& stagingBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		if (stagingBuffer.Size > m_MaxPoolEntrySize)
		{
			// A given stagung buffer was allocated separatly
			auto it = std::find_if(
				m_SeparateAllocations.begin(),
				m_SeparateAllocations.end(),
				[&stagingBuffer](const SeparateBufferAllocation& entry) -> bool
				{
					return entry.Buffer == stagingBuffer.Buffer;
				});

			FLARE_CORE_ASSERT(it != m_SeparateAllocations.end());

			ReleaseSeparateAllocationBuffer(*it);
			m_SeparateAllocations.erase(it);

			return;
		}

		auto it = std::find_if(m_PoolEntries.begin(), m_PoolEntries.end(), [&stagingBuffer](const PoolEntry& entry) -> bool
			{
				return entry.Buffer == stagingBuffer.Buffer;
			});

		FLARE_CORE_ASSERT(it != m_PoolEntries.end());
		FLARE_CORE_ASSERT(it->BytesAllocated == stagingBuffer.Offset + stagingBuffer.Size);

		it->BytesAllocated -= stagingBuffer.Size;
		
		stagingBuffer = {};
	}

	void VulkanStagingBufferPool::FlushMemory()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		VmaAllocator allocator = VulkanContext::GetInstance().GetMemoryAllocator();

		for (auto& entry : m_PoolEntries)
		{
			vmaFlushAllocation(allocator, entry.BufferAllocation.Handle, 0, (VkDeviceSize)entry.BytesAllocated);
		}

		for (auto& separateAllocation : m_SeparateAllocations)
		{
			vmaFlushAllocation(allocator, separateAllocation.Allocation.Handle, 0, (VkDeviceSize)separateAllocation.Size);
		}
	}

	void VulkanStagingBufferPool::Reset()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		ReleaseSeparateAllocationBuffers();
		ReleasePoolEntries(m_MaxReservedPoolEntries, m_PoolEntries.size());

		for (auto& entry : m_PoolEntries)
		{
			entry.BytesAllocated = 0;
		}
	}

	void VulkanStagingBufferPool::Release()
	{
		FLARE_PROFILE_FUNCTION();
		ReleaseSeparateAllocationBuffers();
		ReleasePoolEntries(0, m_PoolEntries.size());

		m_PoolEntries.clear();
	}

	void VulkanStagingBufferPool::CreateBufferEntry()
	{
		FLARE_PROFILE_FUNCTION();
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VmaAllocator allocator = VulkanContext::GetInstance().GetMemoryAllocator();

		PoolEntry& entry = m_PoolEntries.emplace_back();
		entry.BytesAllocated = 0;
		entry.Size = m_MaxPoolEntrySize;

		AllocateBuffer(entry.Size, entry.Buffer, entry.BufferAllocation);

		VK_CHECK_RESULT(vmaMapMemory(allocator, entry.BufferAllocation.Handle, &entry.Mapped));
	}

	void VulkanStagingBufferPool::AllocateBuffer(size_t size, VkBuffer& buffer, VulkanAllocation& allocation)
	{
		FLARE_PROFILE_FUNCTION();
		VmaAllocator allocator = VulkanContext::GetInstance().GetMemoryAllocator();

		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.size = (VkDeviceSize)size;
		createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		VK_CHECK_RESULT(vmaCreateBuffer(
			allocator,
			&createInfo,
			&allocationInfo,
			&buffer,
			&allocation.Handle,
			&allocation.Info));
	}

	void VulkanStagingBufferPool::ReleaseSeparateAllocationBuffers()
	{
		FLARE_PROFILE_FUNCTION();

		for (auto& allocation : m_SeparateAllocations)
		{
			ReleaseSeparateAllocationBuffer(allocation);
		}

		m_SeparateAllocations.clear();
	}

	void VulkanStagingBufferPool::ReleaseSeparateAllocationBuffer(SeparateBufferAllocation& separateAllocation)
	{
		FLARE_PROFILE_FUNCTION();
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VmaAllocator allocator = VulkanContext::GetInstance().GetMemoryAllocator();

		if (separateAllocation.Mapped != nullptr)
		{
			vmaUnmapMemory(allocator, separateAllocation.Allocation.Handle);
			separateAllocation.Mapped = nullptr;
		}

		vmaFreeMemory(allocator, separateAllocation.Allocation.Handle);
		vkDestroyBuffer(device, separateAllocation.Buffer, nullptr);

		separateAllocation = {};
	}

	void VulkanStagingBufferPool::ReleasePoolEntries(size_t rangeStart, size_t rangeEnd)
	{
		FLARE_PROFILE_FUNCTION();
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VmaAllocator allocator = VulkanContext::GetInstance().GetMemoryAllocator();

		if (rangeStart >= rangeEnd)
			return;

		for (size_t i = rangeStart; i < rangeEnd; i++)
		{
			PoolEntry& entry = m_PoolEntries[i];

			vmaUnmapMemory(allocator, entry.BufferAllocation.Handle);
			vkDestroyBuffer(device, entry.Buffer, nullptr);
			vmaFreeMemory(allocator, entry.BufferAllocation.Handle);
		}

		m_PoolEntries.erase(m_PoolEntries.begin() + rangeStart, m_PoolEntries.begin() + rangeEnd);
	}
}
