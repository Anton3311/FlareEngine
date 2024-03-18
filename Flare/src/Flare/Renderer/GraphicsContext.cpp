#include "GraphicsContext.h"

#include "Flare/Platform/OpenGL/OpenGLGraphicsContext.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	Scope<GraphicsContext> GraphicsContext::Create(void* windowHandle)
	{
		VulkanContext context((GLFWwindow*)windowHandle);
		context.Initialize();

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateScope<OpenGLGraphicsContext>((GLFWwindow*) windowHandle);
		}
		return nullptr;
	}
}