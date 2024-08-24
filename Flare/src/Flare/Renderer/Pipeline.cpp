#include "Pipeline.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"

namespace Flare
{
	Ref<Pipeline> Pipeline::Create(const PipelineSpecifications& specifications)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return Ref<VulkanPipeline>::New(specifications);
		}

		return nullptr;
	}
}
