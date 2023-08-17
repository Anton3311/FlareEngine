#include "RendererAPI.h"

#include "Flare/Core/Assert.h"
#include "Flare/Platform/OpenGL/OpenGLRendererAPI.h"

namespace Flare
{
	RendererAPI::API s_API = RendererAPI::API::OpenGL;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
		case API::OpenGL:
			return CreateScope<OpenGLRendererAPI>();
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