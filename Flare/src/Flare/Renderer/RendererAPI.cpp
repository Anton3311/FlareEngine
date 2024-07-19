#include "RendererAPI.h"

#include "FlareCore/Assert.h"

namespace Flare
{
	RendererAPI::API s_API = RendererAPI::API::Vulkan;

	void RendererAPI::Create(RendererAPI::API api)
	{
		s_API = api;
	}

	RendererAPI::API RendererAPI::GetAPI()
	{
		return s_API;
	}
}