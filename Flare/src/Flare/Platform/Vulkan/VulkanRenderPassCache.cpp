#include "VulkanRenderPassCache.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	void VulkanRenderPassCache::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		m_Cache.clear();
	}

	Ref<VulkanRenderPass> VulkanRenderPassCache::GetOrCreate(const VulkanRenderPassKey& key)
	{
		FLARE_PROFILE_FUNCTION();
		auto it = m_Cache.find(key);

		if (it != m_Cache.end())
		{
			return it->second;
		}

		auto renderPass = CreateUniqueRenderPass(key);
		m_Cache.emplace(key, renderPass);

		return renderPass;
	}

	Ref<VulkanRenderPass> VulkanRenderPassCache::CreateUniqueRenderPass(const VulkanRenderPassKey& key)
	{
		FLARE_PROFILE_FUNCTION();
		m_AttachmentDescriptions.clear();

		bool hasClearValues = false;
		for (const auto& attachment : key.Attachments)
			hasClearValues |= attachment.HasClearValue;

		std::optional<uint32_t> depthAttachmentIndex = {};
		for (size_t i = 0; i < key.Attachments.size(); i++)
		{
			VkAttachmentDescription& description = m_AttachmentDescriptions.emplace_back();
			TextureFormat format = key.Attachments[i].Format;

			bool isDepthAttachment = IsDepthTextureFormat(format);
			if (isDepthAttachment)
			{
				FLARE_CORE_ASSERT(!depthAttachmentIndex);
				depthAttachmentIndex = (uint32_t)i;
			}

			description.format = TextureFormatToVulkanFormat(format);
			description.flags = 0;
			description.initialLayout = ImageLayoutToVulkanImageLayout(key.Attachments[i].InitialLayout, format);
			description.finalLayout = ImageLayoutToVulkanImageLayout(key.Attachments[i].FinalLayout, format);
			description.samples = VK_SAMPLE_COUNT_1_BIT;
			description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			if (hasClearValues)
			{
				description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			}

			if (description.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED && !isDepthAttachment)
			{
				description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}

			FLARE_CORE_ASSERT(description.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED);
		}

		return CreateRef<VulkanRenderPass>(
			Span<VkAttachmentDescription>::FromVector(m_AttachmentDescriptions),
			depthAttachmentIndex);
	}
}
