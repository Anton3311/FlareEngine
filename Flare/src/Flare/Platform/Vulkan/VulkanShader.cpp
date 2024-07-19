#include "VulkanShader.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/ShaderCacheManager.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"

namespace Flare
{
	VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Int:
			return VK_FORMAT_R32_SINT;
		case ShaderDataType::Int2:
			return VK_FORMAT_R32G32_SINT;
		case ShaderDataType::Int3:
			return VK_FORMAT_R32G32B32_SINT;
		case ShaderDataType::Int4:
			return VK_FORMAT_R32G32B32A32_SINT;

		case ShaderDataType::Float:
			return VK_FORMAT_R32_SFLOAT;
		case ShaderDataType::Float2:
			return VK_FORMAT_R32G32_SFLOAT;
		case ShaderDataType::Float3:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case ShaderDataType::Float4:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case ShaderDataType::Sampler:
		case ShaderDataType::SamplerArray:
		case ShaderDataType::Matrix4x4:
			FLARE_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

		FLARE_CORE_ASSERT(false);
		return VK_FORMAT_UNDEFINED;
	}

	VulkanShader::~VulkanShader()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		for (auto& stageModule : m_Modules)
		{
			vkDestroyShaderModule(VulkanContext::GetInstance().GetDevice(), stageModule.Module, nullptr);
		}

		vkDestroyPipelineLayout(VulkanContext::GetInstance().GetDevice(), m_PipelineLayout, nullptr);
	}

	VkShaderModule VulkanShader::GetModuleForStage(ShaderStageType stage) const
	{
		for (const auto& stageModule : m_Modules)
		{
			if (stageModule.Stage == stage)
				return stageModule.Module;
		}

		return VK_NULL_HANDLE;
	}

	void VulkanShader::Load()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(Handle));

		m_Valid = false;

		m_Modules.resize(2);
		m_Modules[0].Stage = ShaderStageType::Vertex;
		m_Modules[1].Stage = ShaderStageType::Pixel;

		bool hasValidCache = true;
		for (const ShaderStageModule& stageModule : m_Modules)
		{
			if (!ShaderCacheManager::GetInstance()->HasCache(Handle, stageModule.Stage))
			{
				hasValidCache = false;
				break;
			}
		}

		if (!hasValidCache)
			return;

		m_NameToIndex.clear();
		m_Valid = true;

		m_Metadata = ShaderCacheManager::GetInstance()->FindShaderMetadata(Handle);
		m_Valid = m_Metadata != nullptr;

		if (!m_Valid)
			return;

		const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(Handle);
		FLARE_CORE_ASSERT(assetMetadata);

		m_DebugName = assetMetadata->Path.filename().replace_extension().generic_string();

		for (ShaderStageModule& stageModule : m_Modules)
		{
			auto cachedShader = ShaderCacheManager::GetInstance()->FindCache(Handle, stageModule.Stage);

			if (!cachedShader.has_value())
			{
				FLARE_CORE_ERROR("Failed to find cached Vulkan shader code");

				m_Valid = false;
				break;
			}

			std::string_view stageName = "";
			switch (stageModule.Stage)
			{
			case ShaderStageType::Vertex:
				stageName = "Vertex";
				break;
			case ShaderStageType::Pixel:
				stageName = "Pixel";
				break;
			}

			VkShaderModuleCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.codeSize = cachedShader->size() * sizeof(decltype(cachedShader)::value_type::value_type);
			info.pCode = cachedShader->data();

			VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &stageModule.Module));

			VulkanContext::GetInstance().SetDebugName(
				VK_OBJECT_TYPE_SHADER_MODULE,
				(uint64_t)stageModule.Module,
				fmt::format("{}.{}", m_DebugName, stageName).c_str());
		}

		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			for (const auto& descriptorProperty : m_Metadata->DescriptorProperties)
			{
				m_DescriptorSetsUsageMask |= (1 << descriptorProperty.Set);

				if (descriptorProperty.Set != GetMaterialDescriptorSetIndex(m_Metadata->Type))
					continue;

				VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
				binding = {};
				binding.binding = descriptorProperty.Binding;
				binding.descriptorCount = descriptorProperty.DescriptorCount;
				binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				switch (descriptorProperty.Type)
				{
				case ShaderDescriptorType::UniformBuffer:
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					break;
				case ShaderDescriptorType::Sampler:
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					break;
				case ShaderDescriptorType::StorageBuffer:
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
				case ShaderDescriptorType::StorageImage:
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					break;
				default:
					FLARE_CORE_ASSERT(false, "Unhandled descriptor type");
				}
			}

			if (bindings.size() > 0)
			{
				constexpr size_t maxShaderDescriptorSets = 512;
				m_SetPool = CreateRef<VulkanDescriptorSetPool>(
					maxShaderDescriptorSets,
					Span<VkDescriptorSetLayoutBinding>::FromVector(bindings));
			}
		}

		Ref<const VulkanDescriptorSetLayout> cameraDescriptorLayout = As<const VulkanDescriptorSetLayout>(Renderer::GetCameraDescriptorSetPool()->GetLayout());
		Ref<const VulkanDescriptorSetLayout> globalDescriptorSetLayout = As<const VulkanDescriptorSetLayout>(Renderer::GetGlobalDescriptorSetPool()->GetLayout());
		Ref<const VulkanDescriptorSetLayout> instanceDescriptorLayout = As<const VulkanDescriptorSetLayout>(Renderer::GetInstanceDataDescriptorSetPool()->GetLayout());
		Ref<const VulkanDescriptorSetLayout> emptyDescriptorSetLayout = As<const VulkanDescriptorSetLayout>(VulkanContext::GetInstance().GetEmptyDescriptorSetLayout());

		std::vector<VkPushConstantRange> pushConstantsRanges;
		VkDescriptorSetLayout descriptorSetLayouts[4] = { nullptr };

		if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 0))
			descriptorSetLayouts[0] = cameraDescriptorLayout->GetHandle();

		switch (m_Metadata->Type)
		{
		case ShaderType::Decal:
			if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 1))
				descriptorSetLayouts[1] = As<const VulkanDescriptorSetLayout>(Renderer::GetDecalsDescriptorSetLayout())->GetHandle();
			if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 2))
				descriptorSetLayouts[2] = instanceDescriptorLayout->GetHandle();
			break;
		case ShaderType::Surface:
			if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 1))
				descriptorSetLayouts[1] = globalDescriptorSetLayout->GetHandle();
			if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 2))
				descriptorSetLayouts[2] = instanceDescriptorLayout->GetHandle();
			break;
		case ShaderType::FullscreenQuad:
			if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 1))
				descriptorSetLayouts[1] = globalDescriptorSetLayout->GetHandle();
			break;
		case ShaderType::_2D:
			if (HAS_BIT(m_DescriptorSetsUsageMask, 1 << 1))
				descriptorSetLayouts[1] = As<const VulkanDescriptorSetLayout>(Renderer2D::GetDescriptorSetLayout())->GetHandle();
			break;
		}

		if (m_SetPool)
		{
			descriptorSetLayouts[3] = As<const VulkanDescriptorSetLayout>(m_SetPool->GetLayout())->GetHandle();
		}

		for (size_t i = 0; i < m_Metadata->PushConstantsRanges.size(); i++)
		{
			if (m_Metadata->PushConstantsRanges[i].Size == 0)
				continue;

			VkPushConstantRange& range = pushConstantsRanges.emplace_back();
			switch (m_Metadata->PushConstantsRanges[i].Stage)
			{
			case ShaderStageType::Vertex:
				range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStageType::Pixel:
				range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			}

			range.offset = (uint32_t)m_Metadata->PushConstantsRanges[i].Offset;
			range.size = (uint32_t)m_Metadata->PushConstantsRanges[i].Size;
		}

		uint32_t setLayoutCount = 0;
		for (uint32_t i = 0; i < 4; i++)
		{
			if (descriptorSetLayouts[i] != nullptr)
				setLayoutCount = glm::max(setLayoutCount, i + 1);
		}

		for (uint32_t i = 0; i < setLayoutCount; i++)
		{
			if (descriptorSetLayouts[i] == nullptr)
				descriptorSetLayouts[i] = emptyDescriptorSetLayout->GetHandle();
		}

		// Create pipeline layout
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = setLayoutCount;
		layoutInfo.pSetLayouts = descriptorSetLayouts;
		layoutInfo.pushConstantRangeCount = (uint32_t)pushConstantsRanges.size();
		layoutInfo.pPushConstantRanges = pushConstantsRanges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::GetInstance().GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout));

		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)m_PipelineLayout, m_Metadata->Name.c_str());

		if (!m_Valid)
			return;

		const auto& properties = m_Metadata->Properties;
		for (size_t i = 0; i < properties.size(); i++)
			m_NameToIndex.emplace(properties[i].Name, (uint32_t)i);
	}

	bool VulkanShader::IsLoaded() const
	{
		return m_Valid;
	}

	Ref<const ShaderMetadata> VulkanShader::GetMetadata() const
	{
		return m_Metadata;
	}

	const ShaderProperties& VulkanShader::GetProperties() const
	{
		return m_Metadata->Properties;
	}

	const ShaderOutputs& VulkanShader::GetOutputs() const
	{
		return m_Metadata->Outputs;
	}

	ShaderFeatures VulkanShader::GetFeatures() const
	{
		return m_Metadata->Features;
	}

	std::optional<uint32_t> VulkanShader::GetPropertyIndex(std::string_view name) const
	{
		auto it = m_NameToIndex.find(name);
		if (it == m_NameToIndex.end())
			return {};
		return it->second;
	}

	Ref<const VulkanDescriptorSetLayout> VulkanShader::GetDescriptorSetLayout() const
	{
		return As<const VulkanDescriptorSetLayout>(m_SetPool->GetLayout());
	}
}
