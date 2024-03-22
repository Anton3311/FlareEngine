#include "VulkanBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanBuffer::VulkanBuffer(GPUBufferUsage usage, VkBufferUsageFlags bufferUsage, size_t size)
		: m_Usage(usage), m_UsageFlags(bufferUsage), m_Size(size)
	{
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
			vkUnmapMemory(device, m_BufferMemory);
			m_Mapped = nullptr;
		}

		vkFreeMemory(device, m_BufferMemory, nullptr);
		vkDestroyBuffer(device, m_Buffer, nullptr);
	}

	void VulkanBuffer::SetData(const void* data, size_t size, size_t offset)
	{
		if (m_Size == 0)
			m_Size = size;

		FLARE_CORE_ASSERT(size <= m_Size);

		if (!m_Buffer)
			Create();

		if (m_Usage == GPUBufferUsage::Dynamic)
		{
			FLARE_CORE_ASSERT(m_Mapped);

			std::memcpy(m_Mapped, data, size);
		}
		else
		{
			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

			VkDevice device = VulkanContext::GetInstance().GetDevice();
			VulkanContext::GetInstance().CreateBuffer(size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				stagingBuffer, stagingBufferMemory);

			void* mapped = nullptr;
			VK_CHECK_RESULT(vkMapMemory(device, stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &mapped));

			std::memcpy(mapped, data, size);

			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
			commandBuffer->CopyBuffer(stagingBuffer, m_Buffer, size, 0, offset);
			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);

			vkUnmapMemory(device, stagingBufferMemory);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
			vkDestroyBuffer(device, stagingBuffer, nullptr);
		}
	}

	void VulkanBuffer::Create()
	{
		FLARE_CORE_ASSERT(m_Size > 0);
		VkMemoryPropertyFlags memoryProperties = 0;

		switch (m_Usage)
		{
		case GPUBufferUsage::Static:
			memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			break;
		case GPUBufferUsage::Dynamic:
			memoryProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			break;
		}

		VulkanContext::GetInstance().CreateBuffer(m_Size, m_UsageFlags, memoryProperties, m_Buffer, m_BufferMemory);

		if (m_Usage == GPUBufferUsage::Dynamic)
		{
			VK_CHECK_RESULT(vkMapMemory(VulkanContext::GetInstance().GetDevice(), m_BufferMemory, 0, VK_WHOLE_SIZE, 0, &m_Mapped));
		}
	}
}
