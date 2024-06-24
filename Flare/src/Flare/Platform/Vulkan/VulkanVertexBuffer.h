#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Platform/Vulkan/VulkanBuffer.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(size_t size);
		VulkanVertexBuffer(size_t size, GPUBufferUsage usage);
		VulkanVertexBuffer(const void* data, size_t size);
		VulkanVertexBuffer(const void* data, size_t size, Ref<CommandBuffer> commandBuffer);
		~VulkanVertexBuffer();

		void SetData(const void* data, size_t size, size_t offset) override;
		void SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer) override;

		void SetDebugName(std::string_view debugName) override;
		const std::string& GetDebugName() const override;

		inline VulkanBuffer& GetBuffer() { return m_Buffer; }
		inline const VulkanBuffer& GetBuffer() const { return m_Buffer; }
		inline VkBuffer GetHandle() const { return m_Buffer.GetBuffer(); }
	private:
		VulkanBuffer m_Buffer;
	};
}
