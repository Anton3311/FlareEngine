#pragma once

#include "FlareCore/Collections/Span.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Flare
{
	class VulkanFrameBuffer
	{
	public:
		VulkanFrameBuffer(uint32_t width, uint32_t height, const Ref<VulkanRenderPass>& compatibleRenderPass, const Span<VkImageView>& imageViews);
		~VulkanFrameBuffer();

		VkImageView GetAttachmentImageView(uint32_t index) const
		{
			FLARE_CORE_ASSERT(index < (uint32_t)m_AttachmentsImageViews.size());
			return m_AttachmentsImageViews[index];
		}

		glm::uvec2 GetSize() const { return glm::uvec2(m_Width, m_Height); }

		inline VkFramebuffer GetHandle() const { return m_FrameBuffer; }
	private:
		void Create();
	private:
		bool m_OwnsImageViews = true;

		VkFramebuffer m_FrameBuffer = VK_NULL_HANDLE;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		std::vector<VkImage> m_AttachmentsImages;
		std::vector<VkImageView> m_AttachmentsImageViews;

		Ref<VulkanRenderPass> m_CompatibleRenderPass = nullptr;
	};
}
