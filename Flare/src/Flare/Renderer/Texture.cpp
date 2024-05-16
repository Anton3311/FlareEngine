#include "Texture.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include <fstream>
#include <stb_image/stb_image.h>
#include <dds-ktx/dds-ktx.h>

namespace Flare
{
	FLARE_IMPL_ASSET(Texture);
	FLARE_SERIALIZABLE_IMPL(Texture);

	Ref<Texture> Texture::Create(const TextureSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(specifications);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Texture> Texture::Create(const std::filesystem::path& path, const TextureSpecifications& specifications)
	{
		TextureData data;
		TextureSpecifications newSpecifications = specifications;
		if (!Texture::ReadDataFromFile(path, newSpecifications, data))
		{
			FLARE_CORE_ERROR("Failed to load texture: {}", path.generic_string());
			return nullptr;
		}

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(newSpecifications, data);
		}

		return nullptr;
	}

	Ref<Texture> Texture::Create(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(width, height, data, format, filtering);
		}

		return nullptr;
	}

	Ref<Texture> Texture::Create(const TextureSpecifications& specifications, const TextureData& data)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(specifications, data);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	static TextureFormat ConvertDDSFormatToTextureFormat(ddsktx_format format, uint32_t flags)
	{
		switch (format)
		{
		case DDSKTX_FORMAT_R32F:
			return TextureFormat::RF32;
		case DDSKTX_FORMAT_RGB8:
			return TextureFormat::RGB8;
		case DDSKTX_FORMAT_RGBA8:
			return TextureFormat::RGBA8;
		}

		FLARE_CORE_ASSERT((flags & DDSKTX_TEXTURE_FLAG_SRGB) == 0);

		if (flags % DDSKTX_TEXTURE_FLAG_ALPHA)
		{
			switch (format)
			{
			case DDSKTX_FORMAT_BC1:
				return TextureFormat::BC1_RGBA;
			default:
				FLARE_CORE_ASSERT(false);
			}
		}
		else
		{
			switch (format)
			{
			case DDSKTX_FORMAT_BC1:
				return TextureFormat::BC1_RGB;
			case DDSKTX_FORMAT_BC2:
				return TextureFormat::BC2_RGB;
			case DDSKTX_FORMAT_BC3:
				return TextureFormat::BC3_RGB;
			case DDSKTX_FORMAT_BC4:
				return TextureFormat::BC4_RGB;
			case DDSKTX_FORMAT_BC5:
				return TextureFormat::BC5_RGB;
			default:
				FLARE_CORE_ASSERT(false);
			}
		}

		FLARE_CORE_ASSERT(false);
		return TextureFormat::RGB8;
	}

	static bool ReadDDSFile(const std::filesystem::path& path, TextureSpecifications& specifications, TextureData& data)
	{
		FLARE_PROFILE_FUNCTION();

		uint8_t* fileData = nullptr;
		size_t size = 0;

		{
			FLARE_PROFILE_SCOPE("ReadBinaryFile");
			std::ifstream inputStream(path, std::ios::in | std::ios::binary);

			if (!inputStream.is_open())
				return {};

			inputStream.seekg(0, std::ios::end);
			size = inputStream.tellg();
			inputStream.seekg(0, std::ios::beg);

			fileData = (uint8_t*)malloc(size);

			inputStream.read((char*)fileData, size);
		}

		FLARE_CORE_ASSERT(fileData != nullptr && size > 0);

		ddsktx_texture_info info{ 0 };
		ddsktx_error error{};

		data.Data = fileData;

		if (ddsktx_parse(&info, fileData, (int32_t)size, &error))
		{
			TextureFormat format = ConvertDDSFormatToTextureFormat(info.format, info.flags);

			specifications.Width = (uint32_t)info.width;
			specifications.Height = (uint32_t)info.height;
			specifications.Format = format;

			for (int32_t mip = 0; mip < info.num_mips; mip++)
			{
				ddsktx_sub_data subData;
				ddsktx_get_sub(&info, &subData, fileData, (int32_t)size, 0, 0, mip);

				TextureData::Mip& mipData = data.Mips.emplace_back();
				mipData.Data = subData.buff;
				mipData.SizeInBytes = (size_t)subData.size_bytes;
			}
		}

		return true;
	}

	bool Texture::ReadDataFromFile(const std::filesystem::path& path, TextureSpecifications& specifications, TextureData& data)
	{
		FLARE_PROFILE_FUNCTION();
		if (!std::filesystem::exists(path))
			return false;

		if (path.extension() == ".dds")
		{
			return ReadDDSFile(path, specifications, data);
		}

		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* textureData = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

		if (!textureData)
		{
			return false;
		}

		specifications.Width = (uint32_t)width;
		specifications.Height = (uint32_t)height;
		data.Data = textureData;

		if (channels == 3)
		{
			specifications.Format = TextureFormat::RGB8;
			data.Size = specifications.Width * specifications.Height * 3;
		}
		else if (channels == 4)
		{
			specifications.Format = TextureFormat::RGBA8;
			data.Size = specifications.Width * specifications.Height * 4;
		}
		else if (channels == 2)
		{
			specifications.Format = TextureFormat::RG8;
			data.Size = specifications.Width * specifications.Height * 2;
		}
		else
		{
			FLARE_CORE_ASSERT(false);
		}

		auto& mip = data.Mips.emplace_back();
		mip.Data = data.Data;
		mip.SizeInBytes = data.Size;

		return true;
	}

	bool IsCompressedTextureFormat(TextureFormat format)
	{
		using Underlying = std::underlying_type_t<TextureFormat>;
		return (Underlying)format >= (Underlying)TextureFormat::BC1_RGB;
	}

	const char* TextureFormatToString(TextureFormat format)
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

		case TextureFormat::BC2_RGB:
			return "BC2";
		case TextureFormat::BC3_RGB:
			return "BC3";
		case TextureFormat::BC4_RGB:
			return "BC4";
		case TextureFormat::BC5_RGB:
			return "BC5";
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	bool IsDepthTextureFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::Depth24Stencil8:
		case TextureFormat::Depth32:
			return true;
		}

		return false;
	}

	bool HasStencilComponent(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::Depth24Stencil8:
			return true;
		}

		return false;
	}

	const char* TextureWrapToString(TextureWrap wrap)
	{
		switch (wrap)
		{
		case TextureWrap::Clamp:
			return "Clamp";
		case TextureWrap::Repeat:
			return "Repeat";
		}

		FLARE_CORE_ASSERT(false, "Unahandled texture wrap mode");
		return nullptr;
	}

	const char* TextureFilteringToString(TextureFiltering filtering)
	{
		switch (filtering)
		{
		case TextureFiltering::Linear:
			return "Linear";
		case TextureFiltering::Closest:
			return "Closest";
		}

		FLARE_CORE_ASSERT(false, "Unahandled texture filtering type");
		return nullptr;
	}

	std::optional<TextureWrap> TextureWrapFromString(std::string_view string)
	{
		if (string == "Clamp")
			return TextureWrap::Clamp;
		if (string == "Repeat")
			return TextureWrap::Repeat;
		return {};
	}

	std::optional<TextureFiltering> TextureFilteringFromString(std::string_view string)
	{
		if (string == "Linear")
			return TextureFiltering::Linear;
		if (string == "Closest")
			return TextureFiltering::Closest;
		return {};
	}




	Ref<Texture3D> Texture3D::Create(const Texture3DSpecifications& specifications)
	{
		return nullptr;
	}

	Ref<Texture3D> Texture3D::Create(const Texture3DSpecifications& specifications, const void* data, glm::uvec3 dataSize)
	{
		return nullptr;
	}



	TextureData::~TextureData()
	{
		if (Data)
		{
			free(Data);
			Data = nullptr;
		}
	}
}