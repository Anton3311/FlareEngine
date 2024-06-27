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

	void VulkanRenderPass::SetDebugName(std::string_view debugName)
	{
		m_DebugName = debugName;

		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)m_RenderPass, m_DebugName.c_str());
	}

	const std::string& VulkanRenderPass::GetDebugName() const
	{
		return m_DebugName;
	}

	void VulkanRenderPass::Create(const Span<VkAttachmentDescription>& attachments, std::optional<uint32_t> depthAttachmentIndex)
	{
		FLARE_CORE_ASSERT(!depthAttachmentIndex || depthAttachmentIndex && *depthAttachmentIndex < (uint32_t)attachments.GetSize());

		VkSubpassDependency dependencies[2] = {};

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;

		for (size_t i = 0; i < attachments.GetSize(); i++)
		{
			const auto& attachment = attachments[i];

			bool isDepthAttachment = depthAttachmentIndex.has_value() && *depthAttachmentIndex == (uint32_t)i;
			VkImageLayout attachmentLayout = isDepthAttachment
				? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
				: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (isDepthAttachment)
			{
				dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependencies[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

				dependencies[1].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependencies[1].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			}
			else
			{
				dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

				dependencies[1].dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[1].dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			}

			VulkanContext::GetSourcePipelineStagesAndAccessFlags(attachment.initialLayout, dependencies[0].srcStageMask, dependencies[0].srcAccessMask);
			VulkanContext::GetDestinationPipelineStagesAndAccessFlags(attachmentLayout, dependencies[0].dstStageMask, dependencies[0].dstAccessMask);

			VulkanContext::GetSourcePipelineStagesAndAccessFlags(attachmentLayout, dependencies[1].srcStageMask, dependencies[1].srcAccessMask);
			VulkanContext::GetDestinationPipelineStagesAndAccessFlags(attachment.finalLayout, dependencies[1].dstStageMask, dependencies[1].dstAccessMask);
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
		info.dependencyCount = 2;
		info.pDependencies = dependencies;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;

		VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_RenderPass));
	}
}
