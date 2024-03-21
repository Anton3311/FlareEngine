#include "Shader.h"

#include "Flare/Platform/OpenGL/OpenGLShader.h"
#include "Flare/Platform/Vulkan/VulkanShader.h"
#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	FLARE_IMPL_ASSET(Shader);
	FLARE_SERIALIZABLE_IMPL(Shader);

	Ref<Shader> Shader::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLShader>();
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanShader>();
		}

		return nullptr;
	}
}
