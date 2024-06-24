#include "VulkanRenderPass.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanRenderPass::VulkanRenderPass(const Span<VkAttachmentDescription>& attachments, std::optional<uint32_t> depthAttachmentIndex)
	{
		Create(attachments, depthAttachmentIndex);
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		vkDestroyRenderPass(VulkanContext::GetInstance().GetDevice(), m_RenderPass, nullptr);
	}

	void VulkanRenderPass::SetDefaultClearValues(Span<VkClearValue> clearValues)
	{
		m_DefaultClearValues.assign(clearValues.begin(), clearValues.end());
	}

	static void GetStageAndAccessFlags(VkImageLayout layout, VkPipelineStageFlags& pipelineStages, VkAccessFlags& access)
	{
		switch (layout)
		{
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			pipelineStages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			pipelineStages |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			access |= VK_ACCESS_SHADER_READ_BIT;
			pipelineStages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		}
	}

	void VulkanRenderPass::Create(const Span<VkAttachmentDescription>& attachments, std::optional<uint32_t> depthAttachmentIndex)
	{
		FLARE_CORE_ASSERT(!depthAttachmentIndex || depthAttachmentIndex && *depthAttachmentIndex < (uint32_t)attachments.GetSize());

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		for (const auto& attachment : attachments)
		{
			GetStageAndAccessFlags(attachment.initialLayout, dependency.srcStageMask, dependency.srcAccessMask);
			GetStageAndAccessFlags(attachment.finalLayout, dependency.dstStageMask, dependency.dstAccessMask);
		}

		VkAttachmentReference depthAttachment{};
		if (depthAttachmentIndex)
		{
			depthAttachment.attachment = *depthAttachmentIndex;
			depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		std::vector<VkAttachmentReference> colorAttachments;
		for (size_t i = 0; i < attachments.GetSize(); i++)
		{
			if (depthAttachmentIndex && i == *depthAttachmentIndex)
				continue;

			VkAttachmentReference reference{};
			reference.attachment = (uint32_t)i;
			reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			colorAttachments.push_back(reference);
		}

		m_ColorAttachmentsCount = (uint32_t)colorAttachments.size();

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)colorAttachments.size();
		subpass.pColorAttachments = colorAttachments.data();

		if (depthAttachmentIndex)
		{
			subpass.pDepthStencilAttachment = &depthAttachment;
		}

		VkRenderPassCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.dependencyCount = 1;
		info.flags = 0;
		info.attachmentCount = (uint32_t)attachments.GetSize();
		info.pAttachments = attachments.GetData();
		info.dependencyCount = 1;
		info.pDependencies = &dependency;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;

		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_RenderPass));
	}
}
