#include "VulkanMaterial.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

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
		FLARE_PROFILE_FUNCTION();
		ReleaseDescriptorSet();

		Material::SetShader(shader);

		if (shader == nullptr)
			return;

		m_Pipeline = nullptr;
		m_IsDirty = true;
	}

	Ref<VulkanPipeline> VulkanMaterial::GetPipeline(const Ref<VulkanRenderPass>& renderPass)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Pipeline != nullptr && m_Pipeline->GetCompatibleRenderPass() == renderPass)
			return m_Pipeline;

		m_Pipeline = VulkanContext::GetInstance().GetDefaultPipelineForShader(m_Shader, renderPass).As<VulkanPipeline>();
		return m_Pipeline;
	}

	void VulkanMaterial::UpdateDescriptorSet()
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_IsDirty)
			return;

		if (m_Set != nullptr)
		{
			// Delete the current descriptor set when it's no longer used.
			VulkanContext::GetInstance().EnqueueDescriptorRelease(m_Set, m_Shader.As<VulkanShader>()->GetDescriptorSetPool());
		}

		// Allocate a new descriptor set, because the current one is used in rendering and cannot be updated.
		m_Set = AllocateDescriptorSet();

		if (m_Set == nullptr)
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

	Ref<VulkanDescriptorSet> VulkanMaterial::AllocateDescriptorSet() const
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Shader);

		Ref<VulkanShader> vulkanShader = m_Shader.As<VulkanShader>();
		auto pool = vulkanShader->GetDescriptorSetPool();

		if (pool == nullptr)
			return nullptr;

		Ref<VulkanDescriptorSet> set = nullptr;

		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(Handle);

		set = pool->AllocateSet().As<VulkanDescriptorSet>();

		if (metadata != nullptr)
			set->SetDebugName(metadata->Name);
		else
			set->SetDebugName(m_Shader.As<VulkanShader>()->GetDebugName());

		return set;
	}

	void VulkanMaterial::ReleaseDescriptorSet()
	{
		FLARE_PROFILE_FUNCTION();
		if (!m_Shader || !m_Set)
			return;

		const auto& pool = m_Shader.As<VulkanShader>()->GetDescriptorSetPool();
		if (pool)
		{
			pool->ReleaseSet(m_Set);
		}
	}
}
