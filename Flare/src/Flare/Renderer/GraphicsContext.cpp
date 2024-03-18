#include "GraphicsContext.h"

#include "Flare/Platform/OpenGL/OpenGLGraphicsContext.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	Scope<GraphicsContext> GraphicsContext::Create(void* windowHandle)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateScope<OpenGLGraphicsContext>((GLFWwindow*) windowHandle);
		case RendererAPI::API::Vulkan:
			return CreateScope<VulkanContext>((GLFWwindow*) windowHandle);
		}
		return nullptr;
	}
}