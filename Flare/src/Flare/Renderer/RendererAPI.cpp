#include "RendererAPI.h"

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
		}
	}

	RendererAPI::API RendererAPI::GetAPI()
	{
		return s_API;
	}
}