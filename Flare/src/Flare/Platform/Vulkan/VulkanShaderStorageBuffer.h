#pragma once

#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Platform/Vulkan/VulkanBuffer.h"

namespace Flare
{
	class VulkanShaderStorageBuffer : public ShaderStorageBuffer
	{
	public:
		VulkanShaderStorageBuffer(size_t size);
	
		size_t GetSize() const override;
		void SetData(const MemorySpan& data) override;
        void SetData(const MemorySpan& data, size_t offset, Ref<CommandBuffer> commandBuffer) override;

        void SetDebugName(std::string_view name) override;
        const std::string& GetDebugName() const override;

		inline VkBuffer GetBufferHandle() const { return m_Buffer.GetBuffer(); }
	private:
		std::string m_DebugName;
		VulkanBuffer m_Buffer;
	};
}
