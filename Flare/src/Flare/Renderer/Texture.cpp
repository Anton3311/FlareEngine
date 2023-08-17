#include "Texture.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLTexture.h"

namespace Flare
{
	Ref<Texture> Texture::Create(const std::filesystem::path& path, TextureFiltering filtering)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(path, filtering);
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
}