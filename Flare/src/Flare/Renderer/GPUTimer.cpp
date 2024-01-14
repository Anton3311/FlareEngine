#include "GPUTimer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLGPUTimer.h"

namespace Flare
{
	Ref<GPUTimer> GPUTimer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLGPUTImer>();
		}

		return nullptr;
	}
}
