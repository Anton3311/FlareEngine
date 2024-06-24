#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Platform/Vulkan/VulkanBuffer.h"

namespace Flare
{
	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(IndexFormat format, size_t count);
		VulkanIndexBuffer(IndexFormat format, size_t count, GPUBufferUsage usage);
		VulkanIndexBuffer(IndexFormat format, const MemorySpan& indices);
		VulkanIndexBuffer(IndexFormat format, const MemorySpan& indices, Ref<CommandBuffer> commandBuffer);

		void SetData(const MemorySpan& indices, size_t offset) override;
		void SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer) override;
		size_t GetCount() const override;
		IndexFormat GetIndexFormat() const override;

		void SetDebugName(std::string_view debugName) override;
		const std::string& GetDebugName() const override;

		inline VkBuffer GetHandle() const { return m_Buffer.GetBuffer(); }
	private:
		size_t m_Count = 0;
		IndexBuffer::IndexFormat m_Format;
		VulkanBuffer m_Buffer;
	};
}
