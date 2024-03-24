#pragma once

#include "Flare/Renderer/Buffer.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanBuffer
	{
	public:
		VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage, size_t size);
		VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage);
		~VulkanBuffer();

		void SetData(const void* data, size_t size, size_t offset);
		void EnsureAllocated();

		inline size_t GetSize() const { return m_Size; }
		inline VkBuffer GetBuffer() const { return m_Buffer; }
	private:
		void Create();
	protected:
		GPUBufferUsage m_Usage = GPUBufferUsage::Static;
		void* m_Mapped = nullptr;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;

		size_t m_Size = 0;

		VkBufferUsageFlags m_UsageFlags = 0;
	};
}
