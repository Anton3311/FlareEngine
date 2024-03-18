#include "VulkanFrameBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanFrameBuffer::VulkanFrameBuffer(uint32_t width, uint32_t height, const Ref<VulkanRenderPass>& compatibleRenderPass, const Span<VkImageView>& imageViews)
		: m_OwnsImageViews(false), m_Width(width), m_Height(height), m_CompatibleRenderPass(compatibleRenderPass)
	{
		m_AttachmentsImageViews.assign(imageViews.begin(), imageViews.end());

		Create();
	}

	VulkanFrameBuffer::~VulkanFrameBuffer()
	{
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		if (m_OwnsImageViews)
		{
			for (VkImageView& view : m_AttachmentsImageViews)
			{
				vkDestroyImageView(device, view, nullptr);
				view = VK_NULL_HANDLE;
			}
		}

		for (VkImage& image : m_AttachmentsImages)
		{
			vkDestroyImage(device, image, nullptr);
			image = VK_NULL_HANDLE;
		}

		vkDestroyFramebuffer(device, m_FrameBuffer, nullptr);
	}

	void VulkanFrameBuffer::Create()
	{
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.width = m_Width;
		info.height = m_Height;
		info.renderPass = m_CompatibleRenderPass->GetHandle();
		info.layers = 1;
		info.attachmentCount = (uint32_t)m_AttachmentsImageViews.size();
		info.pAttachments = m_AttachmentsImageViews.data();
		info.flags = 0;

		VK_CHECK_RESULT(vkCreateFramebuffer(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_FrameBuffer));
	}
}
