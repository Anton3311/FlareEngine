#pragma once

#include "Flare/Renderer/ComputePipeline.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline(const ComputePipelineSpecifications& specifications);
		~VulkanComputePipeline();

		inline VkPipeline GetHandle() const { return m_Pipeline; }

		const ComputePipelineSpecifications& GetSpecifications() const override;
	private:
		ComputePipelineSpecifications m_Specifications;

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}
