#include "RendererAPI.h"

#include "FlareCore/Assert.h"
#include "Flare/Platform/OpenGL/OpenGLRendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanRendererAPI.h"

namespace Flare
{
	RendererAPI::API s_API = RendererAPI::API::OpenGL;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
		case API::OpenGL:
			return CreateScope<OpenGLRendererAPI>();
		case API::Vulkan:
			return CreateScope<VulkanRendererAPI>();
		default:
			FLARE_CORE_ASSERT(false, "Unsupported rendering API");
		}

		return nullptr;
	}

	RendererAPI::API RendererAPI::GetAPI()
	{
		return s_API;
	}
}