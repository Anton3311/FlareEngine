#include "Texture.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLTexture.h"

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
		}

		return nullptr;
	}

	Ref<Texture> Texture::Create(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(width, height, data, format, filtering);
		}

		return nullptr;
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
}