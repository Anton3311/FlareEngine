#pragma once

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Texture.h"

#include <unordered_map>
#include <vulkan/vulkan.h>

namespace Flare
{
	struct RenderPassAttachmentKey
	{
		bool operator==(const RenderPassAttachmentKey& other) const
		{
			return Format == other.Format
				&& InitialLayout == other.InitialLayout
				&& FinalLayout == other.FinalLayout
				&& HasClearValue == other.HasClearValue;
		}

		bool operator!=(const RenderPassAttachmentKey& key) const { return !operator==(key); }

		TextureFormat Format = TextureFormat::RGBA8;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
		bool HasClearValue = false;
	};

	struct VulkanRenderPassKey
	{
		bool operator==(const VulkanRenderPassKey& other) const
		{
			if (Attachments.size() != other.Attachments.size())
				return false;

			for (size_t i = 0; i < Attachments.size(); i++)
			{
				if (Attachments[i] != other.Attachments[i])
					return false;
			}

			return true;
		}

		inline bool operator!=(const VulkanRenderPassKey& other) const { return !operator==(other); }

		std::vector<RenderPassAttachmentKey> Attachments;
	};

	template<typename T>
	constexpr std::underlying_type_t<T> ToUnderlying(T value)
	{
		return (std::underlying_type_t<T>)value;
	}
}

template<>
struct std::hash<Flare::VulkanRenderPassKey>
{
	size_t operator()(const Flare::VulkanRenderPassKey& key) const
	{
		FLARE_PROFILE_FUNCTION();
		using namespace Flare;
		size_t hash = 0;

		for (const RenderPassAttachmentKey& attachment : key.Attachments)
		{
			CombineHashes(hash, ToUnderlying(attachment.Format));
			CombineHashes(hash, ToUnderlying(attachment.InitialLayout));
			CombineHashes(hash, ToUnderlying(attachment.FinalLayout));
			CombineHashes(hash, attachment.HasClearValue);
		}

		return hash;
	}
};

namespace Flare
{
	class VulkanRenderPass;
	class FLARE_API VulkanRenderPassCache
	{
	public:
		void Clear();
		Ref<VulkanRenderPass> GetOrCreate(const VulkanRenderPassKey& key);
	private:
		Ref<VulkanRenderPass> CreateUniqueRenderPass(const VulkanRenderPassKey& key);
	private:
		std::vector<VkAttachmentDescription> m_AttachmentDescriptions;
		std::unordered_map<VulkanRenderPassKey, Ref<VulkanRenderPass>> m_Cache;
	};
}
