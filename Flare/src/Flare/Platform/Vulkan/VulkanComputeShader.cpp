#include "VulkanComputeShader.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "Flare/Renderer/ShaderCacheManager.h"

namespace Flare
{
	VulkanComputeShader::VulkanComputeShader() {}

	VulkanComputeShader::~VulkanComputeShader()
	{
		vkDestroyShaderModule(VulkanContext::GetInstance().GetDevice(), m_Module, nullptr);
		vkDestroyPipelineLayout(VulkanContext::GetInstance().GetDevice(), m_PipelineLayout, nullptr);

		m_Module = VK_NULL_HANDLE;
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	Ref<const ComputeShaderMetadata> VulkanComputeShader::GetMetadata() const
	{
		return m_Metadata;
	}

	void Flare::VulkanComputeShader::Load()
	{
		m_IsLoaded = false;

		bool hasValidCache = ShaderCacheManager::GetInstance()->HasCache(Handle, ShaderTargetEnvironment::Vulkan, ShaderStageType::Compute);
		if (!hasValidCache)
			return;

		m_Metadata = ShaderCacheManager::GetInstance()->FindComputeShaderMetadata(Handle);

		auto cachedShader = ShaderCacheManager::GetInstance()->FindCache(
			Handle, ShaderTargetEnvironment::Vulkan,
			ShaderStageType::Compute);

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

		m_IsLoaded = true;
	}

	bool Flare::VulkanComputeShader::IsLoaded() const
	{
		return m_IsLoaded;
	}

	void VulkanComputeShader::CreatePipelineLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (size_t i = 0; i < m_Metadata->Properties.size(); i++)
		{
			const auto& property = m_Metadata->Properties[i];

			if (property.Type == ShaderDataType::Sampler)
			{
				auto& binding = bindings.emplace_back();
				binding = {};
				binding.binding = property.Binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.pImmutableSamplers = nullptr;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			}
			else if (property.Type == ShaderDataType::StorageImage)
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
			m_SetPool = CreateRef<VulkanDescriptorSetPool>(4, Span(bindings.data(), bindings.size()));
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.offset = (uint32_t)m_Metadata->PushConstantsRange.Offset;
		pushConstantRange.size = (uint32_t)m_Metadata->PushConstantsRange.Size;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		VkDescriptorSetLayout layoutHandle = VK_NULL_HANDLE;
		if (m_SetPool == nullptr)
		{
			createInfo.pSetLayouts = nullptr;
			createInfo.setLayoutCount = 0;
		}
		else
		{
			layoutHandle = As<const VulkanDescriptorSetLayout>(m_SetPool->GetLayout())->GetHandle();
			createInfo.pSetLayouts = &layoutHandle;
			createInfo.setLayoutCount = 1;
		}

		if (pushConstantRange.size > 0)
		{
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
}
