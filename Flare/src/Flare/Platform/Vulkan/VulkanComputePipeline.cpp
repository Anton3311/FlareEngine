#include "VulkanComputePipeline.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanComputeShader.h"

namespace Flare
{
	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineSpecifications& specifications)
		: m_Specifications(specifications)
	{
		FLARE_CORE_ASSERT(specifications.Shader);
		FLARE_CORE_ASSERT(specifications.Shader->IsLoaded());

		Ref<VulkanComputeShader> computeShader = As<VulkanComputeShader>(specifications.Shader);

		
		VkComputePipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = 0;
		info.layout = computeShader->GetPipelineLayoutHandle();

		VkPipelineShaderStageCreateInfo& stageInfo = info.stage;
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pName = "main";
		stageInfo.module = computeShader->GetModule();
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

		VK_CHECK_RESULT(vkCreateComputePipelines(VulkanContext::GetInstance().GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &m_Pipeline));
	}

	VulkanComputePipeline::~VulkanComputePipeline()
	{
		vkDestroyPipeline(VulkanContext::GetInstance().GetDevice(), m_Pipeline, nullptr);
	}

	const ComputePipelineSpecifications& Flare::VulkanComputePipeline::GetSpecifications() const
	{
		return m_Specifications;
	}
}
