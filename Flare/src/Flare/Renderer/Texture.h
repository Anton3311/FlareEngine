#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"
#include "FlareCore/Serialization/TypeInitializer.h"
#include "FlareCore/Serialization/Metadata.h"

#include "Flare/AssetManager/Asset.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <optional>

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

		BC1_RGB,
		BC1_RGBA,

		RG8,
		RG16,

		RF32,

		R8,
	};

	inline const char* TextureFormatToString(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB8:
			return "RGB8";
		case TextureFormat::RGBA8:
			return "RGBA8";
		case TextureFormat::RG8:
			return "RG8";
		case TextureFormat::RG16:
			return "RG16";
		case TextureFormat::RF32:
			return "RF32";
		case TextureFormat::R8:
			return "R8";
		case TextureFormat::BC1_RGB:
			return "BC1_RGB";
		case TextureFormat::BC1_RGBA:
			return "BC1_RGBA";
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	enum class TextureFiltering
	{
		Closest,
		Linear,
	};

	struct TextureSpecifications
	{
		static constexpr uint32_t DefaultMipLevelsCount = 4;

		uint32_t Width = 0;
		uint32_t Height = 0;
		TextureFormat Format = TextureFormat::RGB8;
		TextureFiltering Filtering = TextureFiltering::Linear;
		TextureWrap Wrap = TextureWrap::Clamp;

		bool GenerateMipMaps = false;
	};

	FLARE_API const char* TextureWrapToString(TextureWrap wrap);
	FLARE_API const char* TextureFilteringToString(TextureFiltering filtering);

	FLARE_API std::optional<TextureWrap> TextureWrapFromString(std::string_view string);
	FLARE_API std::optional<TextureFiltering> TextureFilteringFromString(std::string_view string);

	struct FLARE_API TextureData
	{
		struct Mip
		{
			const void* Data = nullptr;
			size_t SizeInBytes = 0;
		};

		~TextureData();

		std::vector<Mip> Mips;
		size_t Size = 0;
		void* Data = nullptr;
	};

	class FLARE_API Texture : public Asset
	{
	public:
		FLARE_ASSET;
		FLARE_SERIALIZABLE;

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
		static Ref<Texture> Create(const std::filesystem::path& path, const TextureSpecifications& specifications);
		static Ref<Texture> Create(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering = TextureFiltering::Linear);
		static Ref<Texture> Create(const TextureSpecifications& specifications, const TextureData& textureData);

		static bool ReadDataFromFile(const std::filesystem::path& path, TextureSpecifications& specifications, TextureData& data);
	};



	struct Texture3DSpecifications
	{
		glm::uvec3 Size;
		TextureFormat Format;
		TextureFiltering Filtering;
		TextureWrap Wrap;
	};

	class FLARE_API Texture3D
	{
	public:
		virtual void Bind(uint32_t slot = 0) = 0;
		virtual void* GetRendererId() const = 0;

		virtual void SetData(const void* data, glm::uvec3 dataSize) = 0;
		virtual const Texture3DSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<Texture3D> Create(const Texture3DSpecifications& specifications);
		static Ref<Texture3D> Create(const Texture3DSpecifications& specifications, const void* data, glm::uvec3 dataSize);
	};
}
