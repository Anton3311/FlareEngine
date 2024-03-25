#pragma once

#include "Flare/Renderer/Pipeline.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecifications& specifications,
			const Ref<VulkanRenderPass>& renderPass,
			const Span<Ref<const VulkanDescriptorSetLayout>>& layouts,
			const Span<ShaderPushConstantsRange>& pushConstantsRanges);
		~VulkanPipeline();

		const PipelineSpecifications& GetSpecifications() const override;

		inline VkPipeline GetHandle() const { return m_Pipeline; }
		inline VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
	private:
		PipelineSpecifications m_Specifications;

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	};
}
