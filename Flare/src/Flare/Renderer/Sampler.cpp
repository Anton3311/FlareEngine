#include "Sampler.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanSampler.h"

namespace Flare
{
	Ref<Sampler> Sampler::Create(const SamplerSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanSampler>(specifications);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
}
