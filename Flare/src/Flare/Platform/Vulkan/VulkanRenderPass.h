#pragma once

#include "FlareCore/Collections/Span.h"

#include <vulkan/vulkan.h>
#include <optional>

namespace Flare
{
	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const Span<VkAttachmentDescription>& attachments, std::optional<uint32_t> depthAttachmentIndex = {});
		~VulkanRenderPass();

		void SetDefaultClearValues(Span<VkClearValue> clearValues);
		const std::vector<VkClearValue>& GetDefaultClearValues() const { return m_DefaultClearValues; }

		inline VkRenderPass GetHandle() const { return m_RenderPass; }
		inline uint32_t GetColorAttachmnetsCount() const { return m_ColorAttachmentsCount; }

		void SetDebugName(std::string_view debugName);
		const std::string& GetDebugName() const;
	private:
		void Create(const Span<VkAttachmentDescription>& attachments, std::optional<uint32_t> depthAttachmentIndex);
	private:
		std::string m_DebugName;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		uint32_t m_ColorAttachmentsCount = 0;

		std::vector<VkClearValue> m_DefaultClearValues;
	};
}
