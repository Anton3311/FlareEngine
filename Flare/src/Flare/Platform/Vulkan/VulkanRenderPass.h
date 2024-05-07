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

		inline VkRenderPass GetHandle() const { return m_RenderPass; }
		inline uint32_t GetColorAttachmnetsCount() const { return m_ColorAttachmentsCount; }
	private:
		void Create(const Span<VkAttachmentDescription>& attachments, std::optional<uint32_t> depthAttachmentIndex);
	private:
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		uint32_t m_ColorAttachmentsCount = 0;
	};
}
