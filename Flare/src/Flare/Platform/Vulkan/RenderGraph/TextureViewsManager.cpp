#include "PCH.h"
#include "TextureViewsManager.h"

#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	TextureViewsManager::TextureViewsManager(const RenderGraphResourceManager& resourceManager)
		: m_ResourceManager(resourceManager)
	{
	}

	TextureViewsManager::~TextureViewsManager()
	{
		Clear();
	}

	void TextureViewsManager::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		VkDevice device = VulkanContext::GetInstance().GetDevice();

		for (const auto& [key, value] : m_SubresourceToViewIndex)
		{
			if (key.Subresource == TextureSubresource::FULL_VIEW)
				continue;

			for (size_t i = value; i < frameInFlightCount + value; i++)
			{
				vkDestroyImageView(device, m_Views[i], nullptr);
			}
		}

		m_Views.clear();
		m_SubresourceToViewIndex.clear();
	}

	static bool ValidateSubresource(const TextureSpecifications& texture, const TextureSubresource& subresource)
	{
		if (subresource == TextureSubresource::FULL_VIEW)
			return true;

		bool baseResult = subresource.BaseArrayLayer < texture.ArrayLayerCount && subresource.BaseMip < texture.MipCount;
		bool countResult = (subresource.BaseArrayLayer + subresource.ArrayLayerCount) <= texture.ArrayLayerCount
			&& (subresource.BaseMip + subresource.MipCount) <= texture.MipCount;

		return baseResult && countResult;
	}

	Span<const VkImageView> TextureViewsManager::GetOrCreate(RenderGraphTextureId texture, const TextureSubresource& subresource)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		TextureSubresourceKey key = { texture, subresource };

		auto it = m_SubresourceToViewIndex.find(key);
		if (it != m_SubresourceToViewIndex.end())
		{
			return Span<const VkImageView>::FromVector(m_Views).Slice(it->second, frameInFlightCount);
		}

		size_t insertIndex = m_Views.size();
		m_Views.resize(m_Views.size() + frameInFlightCount);
		Span<VkImageView> slice = Span<VkImageView>::FromVector(m_Views).Slice(insertIndex, frameInFlightCount);
		CreateImageViews(texture, subresource, slice);

		m_SubresourceToViewIndex.emplace(key, insertIndex);

		return slice;
	}

	void TextureViewsManager::CreateImageViews(RenderGraphTextureId texture, const TextureSubresource& subresource, Span<VkImageView> outViews) const
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		FLARE_CORE_ASSERT(outViews.GetSize() == (size_t)frameInFlightCount);

		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			const VulkanTexture& vulkanTexture = m_ResourceManager.GetTextureForFrameInFlight(texture, i).DerefAs<const VulkanTexture>();
			FLARE_CORE_ASSERT(ValidateSubresource(vulkanTexture.GetSpecifications(), subresource));

			if (subresource == TextureSubresource::FULL_VIEW)
			{
				outViews[i] = vulkanTexture.GetImageViewHandle();
			}
			else
			{
				VkImageViewCreateInfo imageViewInfo{};
				imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				imageViewInfo.flags = 0;
				imageViewInfo.format = TextureFormatToVulkanFormat(vulkanTexture.GetSpecifications().Format);
				imageViewInfo.subresourceRange.baseArrayLayer = subresource.BaseArrayLayer;
				imageViewInfo.subresourceRange.baseMipLevel = subresource.BaseMip;
				imageViewInfo.subresourceRange.layerCount = subresource.ArrayLayerCount;
				imageViewInfo.subresourceRange.levelCount = subresource.MipCount;
				imageViewInfo.subresourceRange.aspectMask = IsDepthTextureFormat(vulkanTexture.GetSpecifications().Format)
					? VK_IMAGE_ASPECT_DEPTH_BIT
					: VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewInfo.image = vulkanTexture.GetImageHandle();

				VkImageView imageView = VK_NULL_HANDLE;
				VK_CHECK_RESULT(vkCreateImageView(VulkanContext::GetInstance().GetDevice(), &imageViewInfo, nullptr, &imageView));

				outViews[i] = imageView;
			}
		}
	}

	void TextureViewsManager::ReleaseImageViews(Span<const VkImageView> imageViews) const
	{
		FLARE_PROFILE_FUNCTION();

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		for (VkImageView imageView : imageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
	}
}
