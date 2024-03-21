#include "FrameBuffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"

namespace Flare
{
	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLFrameBuffer>(specifications);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanFrameBuffer>(specifications);
		}

		return nullptr;
	}
}