#include "VulkanComputeShader.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include "Flare/Renderer/ShaderCacheManager.h"

namespace Flare
{
	VulkanComputeShader::VulkanComputeShader() {}

	VulkanComputeShader::~VulkanComputeShader()
	{
		FLARE_PROFILE_FUNCTION();
		Release();
	}

	const Ref<const ComputeShaderMetadata>& VulkanComputeShader::GetMetadata() const
	{
		return m_Metadata;
	}

	void Flare::VulkanComputeShader::Load()
	{
		FLARE_PROFILE_FUNCTION();
		m_IsLoaded = false;

		bool hasValidCache = ShaderCacheManager::GetInstance()->HasCache(Handle, ShaderStageType::Compute);
		if (!hasValidCache)
			return;

		m_Metadata = ShaderCacheManager::GetInstance()->FindComputeShaderMetadata(Handle);
		auto cachedShader = ShaderCacheManager::GetInstance()->FindCache(Handle, ShaderStageType::Compute);

		if (!cachedShader.has_value())
		{
			FLARE_CORE_ERROR("Failed to find cached Vulkan shader code");
			return;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pCode = cachedShader->data();
		createInfo.codeSize = (uint32_t)(cachedShader->size() * sizeof(uint32_t));

		VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::GetInstance().GetDevice(), &createInfo, nullptr, &m_Module));

		CreatePipelineLayout();
		CreatePipeline();

		m_IsLoaded = true;
	}

	bool Flare::VulkanComputeShader::IsLoaded() const
	{
		return m_IsLoaded;
	}

	void VulkanComputeShader::Release()
	{
		FLARE_PROFILE_FUNCTION();

		VkDevice device = VulkanContext::GetInstance().GetDevice();

		vkDestroyShaderModule(device, m_Module, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		vkDestroyPipeline(device, m_Pipeline, nullptr);

		m_Module = VK_NULL_HANDLE;
		m_PipelineLayout = VK_NULL_HANDLE;
		m_Pipeline = nullptr;
	}

	void VulkanComputeShader::CreatePipelineLayout()
	{
		FLARE_PROFILE_FUNCTION();
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (size_t i = 0; i < m_Metadata->DescriptorProperties.size(); i++)
		{
			const auto& property = m_Metadata->DescriptorProperties[i];
			if (property.Set != 3)
				continue;

			if (property.Type == ShaderDescriptorType::SampledImage)
			{
				auto& binding = bindings.emplace_back();
				binding = {};
				binding.binding = property.Binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.pImmutableSamplers = nullptr;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			}
			else if (property.Type == ShaderDescriptorType::StorageImage)
			{
				auto& binding = bindings.emplace_back();
				binding = {};
				binding.binding = property.Binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.pImmutableSamplers = nullptr;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			}
		}

		if (bindings.size() > 0)
		{
			m_SetPool = Ref<VulkanDescriptorSetPool>::New(Span(bindings.data(), bindings.size()));
		}

		Ref<const VulkanDescriptorSetLayout> emptyDescriptorSetLayout = VulkanContext::GetInstance().GetEmptyDescriptorSetLayout().As<const VulkanDescriptorSetLayout>();

		uint32_t usedDescriptorSetCount = 0;

		VkDescriptorSetLayout layoutHandles[4] = { VK_NULL_HANDLE };
		for (size_t i = 0; i < 4; i++)
		{
			ShaderDescriptorSetUsage usage = m_Metadata->DescriptorSetUsage[i];
			if (usage.Usage == ShaderDescriptorSetUsage::UsageType::NotUsed)
				break;

			if (usage.Usage == ShaderDescriptorSetUsage::UsageType::Empty)
			{
				layoutHandles[i] = emptyDescriptorSetLayout->GetHandle();
			}

			usedDescriptorSetCount = (uint32_t)(i + 1);
		}

		if (m_Metadata->DescriptorSetUsage[0].Usage == ShaderDescriptorSetUsage::UsageType::Used)
		{
			Ref<const VulkanDescriptorSetLayout> cameraDescriptorLayout = Renderer::GetCameraDescriptorSetPool()->GetLayout().As<const VulkanDescriptorSetLayout>();
			layoutHandles[0] = cameraDescriptorLayout->GetHandle();
		}
		
		if (m_Metadata->DescriptorSetUsage[1].Usage == ShaderDescriptorSetUsage::UsageType::Used)
		{
			Ref<const VulkanDescriptorSetLayout> globalDescriptorSetLayout = Renderer::GetGlobalDescriptorSetPool()->GetLayout().As<const VulkanDescriptorSetLayout>();
			layoutHandles[1] = globalDescriptorSetLayout->GetHandle();
		}

		FLARE_CORE_ASSERT(m_Metadata->DescriptorSetUsage[2].Usage != ShaderDescriptorSetUsage::UsageType::Used, "Set 2 is not currently supported for compute shaders");

		if (m_SetPool != nullptr)
		{
			layoutHandles[3] = m_SetPool->GetLayout().As<const VulkanDescriptorSetLayout>()->GetHandle();
		}

		VkPipelineLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.pSetLayouts = layoutHandles;
		createInfo.setLayoutCount = usedDescriptorSetCount;

		VkPushConstantRange pushConstantRange{};
		if (m_Metadata->PushConstantsRanges.size() > 0 && m_Metadata->PushConstantsRanges[0].Size > 0)
		{
			pushConstantRange.offset = (uint32_t)m_Metadata->PushConstantsRanges[0].Offset;
			pushConstantRange.size = (uint32_t)m_Metadata->PushConstantsRanges[0].Size;
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			createInfo.pPushConstantRanges = &pushConstantRange;
			createInfo.pushConstantRangeCount = 1;
		}
		else
		{
			createInfo.pPushConstantRanges = nullptr;
			createInfo.pushConstantRangeCount = 0;
		}

		VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::GetInstance().GetDevice(), &createInfo, nullptr, &m_PipelineLayout));
	}

	void VulkanComputeShader::CreatePipeline()
	{
		FLARE_PROFILE_FUNCTION();

		VkComputePipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = 0;
		info.layout = m_PipelineLayout;

		VkPipelineShaderStageCreateInfo& stageInfo = info.stage;
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pName = "main";
		stageInfo.module = m_Module;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

		VK_CHECK_RESULT(vkCreateComputePipelines(VulkanContext::GetInstance().GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &m_Pipeline));
	}
}
