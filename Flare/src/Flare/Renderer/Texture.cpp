#include "Texture.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLTexture.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include <stb_image/stb_image.h>

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

	Ref<Texture> Texture::Create(const TextureSpecifications& specifications, const void* data)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(specifications, data);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanTexture>(specifications, data);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	bool Texture::ReadDataFromFile(const std::filesystem::path& path, TextureData& data)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* textureData = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

		if (!textureData)
		{
			return false;
		}

		data.Specifications.Width = (uint32_t)width;
		data.Specifications.Height = (uint32_t)height;
		data.Data = textureData;
		
		if (channels == 3)
		{
			data.Specifications.Format = TextureFormat::RGB8;
		}
		else if (channels == 4)
		{
			data.Specifications.Format = TextureFormat::RGBA8;
		}
		else
		{
			FLARE_CORE_ASSERT(false);
		}
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