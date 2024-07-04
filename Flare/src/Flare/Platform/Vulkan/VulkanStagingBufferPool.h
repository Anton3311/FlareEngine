#pragma once

#include "FlareCore/Core.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	struct VulkanStagingBuffer
	{
		VkBuffer Buffer = VK_NULL_HANDLE;
		size_t AllocationOffset = 0;
		size_t Offset = 0;
		size_t Size = 0;

		void* Mapped = nullptr;
	};

	class FLARE_API VulkanStagingBufferPool
	{
	public:
		VulkanStagingBufferPool(size_t maxEntrySize, size_t maxReservedPoolEntries);
		VulkanStagingBuffer AllocateStagingBuffer(size_t size);
		VulkanStagingBuffer AllocateAlignedStagingBuffer(size_t size, size_t alignment);
		
		// Becuase the VulkanStagingBufferPool is implemented as a stack allocator,
		// staging buffers must be released in reverse order they were allocated
		void ReleaseStagingBuffer(VulkanStagingBuffer& stagingBuffer);

		void FlushMemory();
		void Reset();
		void Release();
	private:
		struct PoolEntry
		{
			VkBuffer Buffer = VK_NULL_HANDLE;
			VulkanAllocation BufferAllocation;
			size_t Size = 0;
			size_t BytesAllocated = 0;
			void* Mapped = nullptr;
		};

		struct SeparateBufferAllocation
		{
			VkBuffer Buffer = VK_NULL_HANDLE;
			VulkanAllocation Allocation;
			size_t Size = 0;
			void* Mapped = nullptr;
		};

		void CreateBufferEntry();
		void AllocateBuffer(size_t size, VkBuffer& buffer, VulkanAllocation& allocation);
		void ReleaseSeparateAllocationBuffers();
		void ReleaseSeparateAllocationBuffer(SeparateBufferAllocation& separateAllocation);
		void ReleasePoolEntries(size_t rangeStart, size_t rangeEnd);
	private:
		size_t m_MaxPoolEntrySize = 0;
		size_t m_MaxReservedPoolEntries = 0;
		std::vector<PoolEntry> m_PoolEntries;
		std::vector<SeparateBufferAllocation> m_SeparateAllocations;
	};
}
