#include "PCH.h"

#include "Buffer.h"

#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/Vulkan/VulkanBuffer.h"

namespace Flare
{
	Ref<GPUBuffer> GPUBuffer::Create(const GPUBufferSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return Ref<VulkanBuffer>::New(specifications);
		}

		FLARE_CORE_VERIFY_UNREACHABLE();
		return nullptr;
	}
}