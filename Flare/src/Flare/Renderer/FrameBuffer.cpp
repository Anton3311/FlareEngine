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
			return Ref<VulkanFrameBuffer>::New(attachmentTextures);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return Ref<VulkanFrameBuffer>::New(specifications);
		}

		return nullptr;
	}
}