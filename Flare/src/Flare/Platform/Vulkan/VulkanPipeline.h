#pragma once

#include "Flare/Renderer/Pipeline.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class FLARE_API VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecifications& specifications,
			const Ref<VulkanRenderPass>& renderPass,
			const Span<Ref<const DescriptorSetLayout>>& layouts,
			const Span<ShaderPushConstantsRange>& pushConstantsRanges);
		
		VulkanPipeline(const PipelineSpecifications& specifications,
			const Span<Ref<const DescriptorSetLayout>>& layouts,
			const Span<ShaderPushConstantsRange>& pushConstantsRanges);

		VulkanPipeline(const PipelineSpecifications& specifications, const Ref<VulkanRenderPass>& renderPass);
		VulkanPipeline(const PipelineSpecifications& specifications);
		~VulkanPipeline();

		const PipelineSpecifications& GetSpecifications() const override;

		VkPipeline GetHandle(const Ref<VulkanRenderPass>& renderPass);

		inline VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
		inline Ref<VulkanRenderPass> GetCompatibleRenderPass() const { return m_CompatibleRenderPass; }
	private:
		void CreatePipelineLayout(const Span<Ref<const DescriptorSetLayout>>& layouts, const Span<ShaderPushConstantsRange>& pushConstantsRanges);
		void Create();

		void ReleasePipeline();
	private:
		PipelineSpecifications m_Specifications;
		Ref<VulkanRenderPass> m_CompatibleRenderPass = nullptr;

		bool m_OwnsPipelineLayout = true;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	};
}
