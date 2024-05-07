#include "ComputePipeline.h"

#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/Vulkan/VulkanComputePipeline.h"

namespace Flare
{
	Ref<ComputePipeline> ComputePipeline::Create(const ComputePipelineSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanComputePipeline>(specifications);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
}
