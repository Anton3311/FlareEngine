#include "Texture.h"

#include <Flare/Renderer/RendererAPI.h>

#include <Flare/Platform/OpenGL/OpenGLTexture.h>

namespace Flare
{
	Ref<Texture> Texture::Create(const std::filesystem::path& path)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLTexture>(path);
		}
	}
}