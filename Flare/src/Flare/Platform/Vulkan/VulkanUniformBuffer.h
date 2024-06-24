#pragma once

#include "Flare/Renderer/UniformBuffer.h"

#include "Flare/Platform/Vulkan/VulkanBuffer.h"

namespace Flare
{
	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(size_t size);

		void SetData(const void* data, size_t size, size_t offset) override;
		size_t GetSize() const override;

        void SetDebugName(std::string_view name) override;
        const std::string& GetDebugName() const override;

		inline VkBuffer GetBufferHandle() const { return m_Buffer.GetBuffer(); }
	private:
		VulkanBuffer m_Buffer;
	};
}
