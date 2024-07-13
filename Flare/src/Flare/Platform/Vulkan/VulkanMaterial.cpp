#include "VulkanMaterial.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanShader.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"

namespace Flare
{
	VulkanMaterial::~VulkanMaterial()
	{
		ReleaseDescriptorSet();
	}

	void VulkanMaterial::SetShader(const Ref<Shader>& shader)
	{
		ReleaseDescriptorSet();

		Material::SetShader(shader);

		if (shader == nullptr)
		{
			return;
		}

		m_Pipeline = nullptr;
		m_Set = nullptr;

		Ref<VulkanShader> vulkanShader = As<VulkanShader>(shader);
		auto pool = vulkanShader->GetDescriptorSetPool();

		if (pool)
		{
			m_Set = As<VulkanDescriptorSet>(pool->AllocateSet());

			const AssetMetadata* metadata = AssetManager::GetAssetMetadata(Handle);
			if (metadata != nullptr)
			{
				m_Set->SetDebugName(metadata->Name);
			}
			else
			{
				m_Set->SetDebugName(As<VulkanShader>(m_Shader)->GetDebugName());
			}
		}

		m_IsDirty = true;
	}

	Ref<VulkanPipeline> VulkanMaterial::GetPipeline(const Ref<VulkanRenderPass>& renderPass)
	{
		if (m_Pipeline != nullptr && m_Pipeline->GetCompatibleRenderPass().get() == renderPass.get())
			return m_Pipeline;

		m_Pipeline = As<VulkanPipeline>(VulkanContext::GetInstance().GetDefaultPipelineForShader(m_Shader, renderPass));
		return m_Pipeline;
	}

	void VulkanMaterial::UpdateDescriptorSet()
	{
		if (!m_IsDirty)
			return;

		const auto& properties = m_Shader->GetMetadata()->Properties;
		for (size_t i = 0; i < properties.size(); i++)
		{
			const auto& property = properties[i];
			if (property.Type != ShaderDataType::Sampler)
				continue;

			const auto& texture = GetTextureProperty((uint32_t)i);
			if (texture)
			{
				m_Set->WriteImage(texture, property.Binding);
			}
			else
			{
				FLARE_CORE_WARN("Material has an invalid texture property at index {}. A white texture is used instead", i);
				m_Set->WriteImage(Renderer::GetWhiteTexture(), property.Binding);
			}
		}

		m_Set->FlushWrites();

		m_IsDirty = false;
	}

	void VulkanMaterial::ReleaseDescriptorSet()
	{
		if (!m_Shader || !m_Set)
			return;

		const auto& pool = As<VulkanShader>(m_Shader)->GetDescriptorSetPool();
		if (pool)
		{
			pool->ReleaseSet(m_Set);
		}
	}
}
