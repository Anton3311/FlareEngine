#pragma once

#include "Flare/Renderer/Pipeline.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecifications& specifications, const Ref<VulkanRenderPass>& renderPass);
		~VulkanPipeline();

		const PipelineSpecifications& GetSpecifications() const override;
	private:
		PipelineSpecifications m_Specifications;

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	};
}
