#include "VulkanShader.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/ShaderCacheManager.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

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
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(Handle));

		m_Valid = false;

		m_Modules.resize(2);
		m_Modules[0].Stage = ShaderStageType::Vertex;
		m_Modules[1].Stage = ShaderStageType::Pixel;

		bool hasValidCache = true;
		for (const ShaderStageModule& stageModule : m_Modules)
		{
			if (!ShaderCacheManager::GetInstance()->HasCache(Handle, ShaderTargetEnvironment::Vulkan, stageModule.Stage))
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

		for (ShaderStageModule& stageModule : m_Modules)
		{
			auto cachedShader = ShaderCacheManager::GetInstance()->FindCache(
				Handle, ShaderTargetEnvironment::Vulkan,
				stageModule.Stage);

			if (!cachedShader.has_value())
			{
				FLARE_CORE_ERROR("Failed to find cached Vulkan shader code");

				m_Valid = false;
				break;
			}

			VkShaderModuleCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.codeSize = cachedShader->size() * sizeof(decltype(cachedShader)::value_type::value_type);
			info.pCode = cachedShader->data();

			VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &stageModule.Module));
		}

		if (!m_Valid)
			return;

		const auto& properties = m_Metadata->Properties;
		for (size_t i = 0; i < properties.size(); i++)
			m_NameToIndex.emplace(properties[i].Name, (uint32_t)i);
	}

	bool VulkanShader::IsLoaded()
	{
		return false;
	}

	void VulkanShader::Bind()
	{
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

	void VulkanShader::SetInt(const std::string& name, int value)
	{
	}

	void VulkanShader::SetFloat(const std::string& name, float value)
	{
	}

	void VulkanShader::SetFloat2(const std::string& name, glm::vec2 value)
	{
	}

	void VulkanShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
	}

	void VulkanShader::SetFloat4(const std::string& name, const glm::vec4& value)
	{
	}

	void VulkanShader::SetIntArray(const std::string& name, const int* values, uint32_t count)
	{
	}

	void VulkanShader::SetMatrix4(const std::string& name, const glm::mat4& matrix)
	{
	}
}
