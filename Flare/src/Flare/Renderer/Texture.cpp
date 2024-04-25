#include "Texture.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLTexture.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include <fstream>
#include <stb_image/stb_image.h>
#include <dds-ktx/dds-ktx.h>

namespace Flare
{
	FLARE_IMPL_ASSET(Texture);
	FLARE_SERIALIZABLE_IMPL(Texture);

	Ref<Texture> Texture::Create(const std::filesystem::path& path, const TextureSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(path, specifications);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(path, specifications);
		}

		return nullptr;
	}

	Ref<Texture> Texture::Create(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(width, height, data, format, filtering);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(width, height, data, format, filtering);
		}

		return nullptr;
	}

	Ref<Texture> Texture::Create(const TextureSpecifications& specifications, const TextureData& data)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(specifications, data.Data);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(specifications, data);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	static bool ReadDDSFile(const std::filesystem::path& path, TextureSpecifications& specifications, TextureData& data)
	{
		std::ifstream inputStream(path, std::ios::in | std::ios::binary);

		if (!inputStream.is_open())
			return {};

		inputStream.seekg(0, std::ios::end);
		size_t size = inputStream.tellg();
		inputStream.seekg(0, std::ios::beg);

		uint8_t* fileData = (uint8_t*)malloc(size);

		inputStream.read((char*)fileData, size);

		ddsktx_texture_info info{ 0 };
		ddsktx_error error{};

		data.Data = fileData;

		if (ddsktx_parse(&info, fileData, (int32_t)size, &error))
		{
			TextureFormat format = TextureFormat::RGB8;
			switch (info.format)
			{
			case DDSKTX_FORMAT_BC1:
				if (info.flags & DDSKTX_TEXTURE_FLAG_ALPHA)
					format = TextureFormat::BC1_RGBA;
				else
					format = TextureFormat::BC1_RGB;
				break;
			}

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
		else
		{
			FLARE_CORE_ASSERT(false);
		}

		auto& mip = data.Mips.emplace_back();
		mip.Data = data.Data;
		mip.SizeInBytes = data.Size;

		return true;
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
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture3D>(specifications);
		}

		return nullptr;
	}

	Ref<Texture3D> Texture3D::Create(const Texture3DSpecifications& specifications, const void* data, glm::uvec3 dataSize)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture3D>(specifications, data, dataSize);
		}

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