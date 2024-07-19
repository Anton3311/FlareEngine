#pragma once

#include "Flare/Renderer/Texture.h"

#include <string>
#include <stdint.h>

namespace Flare
{
	struct RenderGraphTextureId
	{
	public:
		RenderGraphTextureId() = default;

		explicit RenderGraphTextureId(uint32_t value)
			: m_Value(value) {}


		constexpr bool operator==(RenderGraphTextureId other) const { return m_Value == other.m_Value; }
		constexpr bool operator!=(RenderGraphTextureId other) const { return m_Value != other.m_Value; }

		constexpr uint32_t GetValue() const { return m_Value; }
	private:
		uint32_t m_Value = UINT32_MAX;
	};
}

template<>
struct std::hash<Flare::RenderGraphTextureId>
{
	size_t operator()(Flare::RenderGraphTextureId id) const
	{
		return std::hash<uint32_t>()(id.GetValue());
	}
};

namespace Flare
{
	struct RenderGraphTextureResource
	{
		std::string DebugName;
		TextureFormat Format = TextureFormat::RGBA8;

		Ref<Texture> Texture = nullptr;

		uint32_t TextureObjectIndex = UINT32_MAX;
	};

	class Viewport;
	class FLARE_API RenderGraphResourceManager
	{
	public:
		RenderGraphResourceManager(const Viewport& viewport);

		RenderGraphTextureId CreateTexture(TextureFormat format, std::string_view debugName);
		RenderGraphTextureId RegisterExistingTexture(Ref<Texture> texture);

		void Clear();

		inline bool IsTextureIdValid(RenderGraphTextureId textureId) const { return textureId.GetValue() < (uint32_t)m_Textures.size(); }
		inline Ref<Texture> GetTexture(RenderGraphTextureId textureId) const
		{
			FLARE_CORE_ASSERT(IsTextureIdValid(textureId));
			return m_Textures[textureId.GetValue()].Texture;
		}

		inline TextureFormat GetTextureFormat(RenderGraphTextureId textureId) const
		{
			FLARE_CORE_ASSERT(IsTextureIdValid(textureId));
			return m_Textures[textureId.GetValue()].Format;
		}

		size_t GetTextureResourceCount() const { return m_Textures.size(); }
	private:
		const Viewport& m_Viewport;
		std::vector<RenderGraphTextureResource> m_Textures;
	};
}
