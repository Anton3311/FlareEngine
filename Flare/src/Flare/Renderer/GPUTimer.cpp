#include "GPUTimer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanGPUTimer.h"

namespace Flare
{
	Ref<GPUTimer> GPUTimer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanGPUTimer>();
		}

		return nullptr;
	}
}
