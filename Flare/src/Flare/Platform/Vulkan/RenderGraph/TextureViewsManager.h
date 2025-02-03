#pragma once

#include "Flare/Renderer/Texture.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanTexture;

	struct TextureSubresourceKey
	{
		RenderGraphTextureId Texture;
		TextureSubresource Subresource;

		constexpr bool operator==(const TextureSubresourceKey& other) const
		{
			return Texture == other.Texture && Subresource == other.Subresource;
		}

		constexpr bool operator!=(const TextureSubresourceKey& other) const
		{
			return !operator==(other);
		}
	};

	struct TextureSubresourceKeyHasher
	{
		inline size_t operator()(const TextureSubresourceKey& key) const
		{
			size_t hash = std::hash<RenderGraphTextureId>()(key.Texture);
			CombineHashes(hash, key.Subresource.BaseArrayLayer);
			CombineHashes(hash, key.Subresource.ArrayLayerCount);
			CombineHashes(hash, key.Subresource.BaseMip);
			CombineHashes(hash, key.Subresource.MipCount);
			return hash;
		}
	};

	class FLARE_API TextureViewsManager
	{
	public:
		TextureViewsManager(const RenderGraphResourceManager& resourceManager);
		~TextureViewsManager();

		void Clear();

		Span<const VkImageView> GetOrCreate(RenderGraphTextureId texture, const TextureSubresource& subResource);

		void RecreateCachedTextureViews(RenderGraphTextureId texture);
	private:
		void CreateImageViews(RenderGraphTextureId texture, const TextureSubresource& subresource, Span<VkImageView> outViews) const;
		void ReleaseImageViews(Span<const VkImageView> imageViews) const;
	private:
		const RenderGraphResourceManager& m_ResourceManager;

		std::unordered_map<TextureSubresourceKey, size_t, TextureSubresourceKeyHasher> m_SubresourceToViewIndex;
		std::vector<VkImageView> m_Views;
	};
}
