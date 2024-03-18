#pragma once

#include "FlareCore/Collections/Span.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const Span<VkAttachmentDescription>& attachments);
		~VulkanRenderPass();

		inline VkRenderPass GetHandle() const { return m_RenderPass; }
	private:
		void Create(const Span<VkAttachmentDescription>& attachments);
	private:
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};
}
