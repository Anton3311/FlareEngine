#pragma once

#include "FlareCore/Core.h"

#include "Flare/AssetManager/Asset.h"

#include <filesystem>

namespace Flare
{
	enum class TextureWrap
	{
		Clamp,
		Repeat,
	};

	enum class TextureFormat
	{
		RGB8,
		RGBA8,
	};

	enum class TextureFiltering
	{
		Closest,
		Linear,
	};

	struct TextureSpecifications
	{
		uint32_t Width;
		uint32_t Height;
		TextureFormat Format;
		TextureFiltering Filtering;
	};

	class FLARE_API Texture : public Asset
	{
	public:
		Texture()
			: Asset(AssetType::Texture) {}

		virtual void Bind(uint32_t slot = 0) = 0;
		virtual void SetData(const void* data, size_t size) = 0;

		virtual const TextureSpecifications& GetSpecifications() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual TextureFormat GetFormat() const = 0;
		virtual TextureFiltering GetFiltering() const = 0;
	public:
		static Ref<Texture> Create(const std::filesystem::path& path, TextureFiltering filtering = TextureFiltering::Linear);
		static Ref<Texture> Create(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering = TextureFiltering::Linear);
	};
}
