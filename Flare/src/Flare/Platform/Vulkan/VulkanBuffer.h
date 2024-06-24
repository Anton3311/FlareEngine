#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class CommandBuffer;
	class VulkanBuffer
	{
	public:
		VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage, size_t size);
		VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage);
		~VulkanBuffer();

		void SetData(const void* data, size_t size, size_t offset);
		void SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer);
		void EnsureAllocated();

		inline size_t GetSize() const { return m_Size; }
		inline VkBuffer GetBuffer() const { return m_Buffer; }

		void SetDebugName(std::string_view name);
		inline const std::string& GetDebugName() const { return m_DebugName; }
	private:
		void Create();
		StagingBuffer FillStagingBuffer(MemorySpan data);
	protected:
		std::string m_DebugName;

		GPUBufferUsage m_Usage = GPUBufferUsage::Static;
		void* m_Mapped = nullptr;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VulkanAllocation m_Allocation;

		size_t m_Size = 0;

		VkBufferUsageFlags m_UsageFlags = 0;
	};
}
