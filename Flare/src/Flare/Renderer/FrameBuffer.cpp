#include "FrameBuffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"

namespace Flare
{
	Ref<FrameBuffer> FrameBuffer::Create(Span<Ref<Texture>> attachmentTextures)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanFrameBuffer>(attachmentTextures);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanFrameBuffer>(specifications);
		}

		return nullptr;
	}
}