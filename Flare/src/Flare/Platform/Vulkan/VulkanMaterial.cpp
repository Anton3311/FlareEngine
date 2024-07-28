#include "VulkanMaterial.h"

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
		{
			return;
		}

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		if ((uint32_t)m_SetStates.size() < frameInFlightCount)
		{
			m_SetStates.resize(frameInFlightCount);
		}

		m_Pipeline = nullptr;
		m_SetStates.assign(m_SetStates.size(), DescriptorSetState{});

		Ref<VulkanShader> vulkanShader = As<VulkanShader>(shader);
		auto pool = vulkanShader->GetDescriptorSetPool();

		if (pool)
		{
			const AssetMetadata* metadata = AssetManager::GetAssetMetadata(Handle);
			for (uint32_t i = 0; i < frameInFlightCount; i++)
			{
				m_SetStates[i].Set = As<VulkanDescriptorSet>(pool->AllocateSet());

				if (metadata != nullptr)
					m_SetStates[i].Set->SetDebugName(metadata->Name);
				else
					m_SetStates[i].Set->SetDebugName(As<VulkanShader>(m_Shader)->GetDebugName());
			}
		}

		m_IsDirty = true;
	}

	Ref<VulkanPipeline> VulkanMaterial::GetPipeline(const Ref<VulkanRenderPass>& renderPass)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Pipeline != nullptr && m_Pipeline->GetCompatibleRenderPass().get() == renderPass.get())
			return m_Pipeline;

		m_Pipeline = As<VulkanPipeline>(VulkanContext::GetInstance().GetDefaultPipelineForShader(m_Shader, renderPass));
		return m_Pipeline;
	}

	Ref<DescriptorSet> VulkanMaterial::GetDescriptorSet() const
	{
		return m_SetStates[GraphicsContext::GetInstance().GetCurrentFrameInFlight()].Set;
	}

	void VulkanMaterial::UpdateDescriptorSet()
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlight = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		if (!m_IsDirty && !m_SetStates[frameInFlight].IsDirty)
		{
			return;
		}

		// NOTE: Material properties were updated during the current frame,
		//       but becuase the material has multiple descriptor sets for each frame in flight,
		//       it is neccessary to propagate these changes to all descriptor sets.
		if (m_IsDirty)
		{
			for (auto& state : m_SetStates)
				state.IsDirty = true;
		}

		const auto& properties = m_Shader->GetMetadata()->Properties;
		for (size_t i = 0; i < properties.size(); i++)
		{
			const auto& property = properties[i];
			if (property.Type != ShaderDataType::Sampler)
				continue;

			const auto& texture = GetTextureProperty((uint32_t)i);
			if (texture)
			{
				m_SetStates[frameInFlight].Set->WriteImage(texture, property.Binding);
			}
			else
			{
				FLARE_CORE_WARN("Material has an invalid texture property at index {}. A white texture is used instead", i);
				m_SetStates[frameInFlight].Set->WriteImage(Renderer::GetWhiteTexture(), property.Binding);
			}
		}

		m_SetStates[frameInFlight].Set->FlushWrites();
		m_SetStates[frameInFlight].IsDirty = false;

		m_IsDirty = false;
	}

	void VulkanMaterial::ReleaseDescriptorSet()
	{
		FLARE_PROFILE_FUNCTION();
		if (!m_Shader)
			return;

		const auto& pool = As<VulkanShader>(m_Shader)->GetDescriptorSetPool();
		if (pool)
		{
			for (const DescriptorSetState& state : m_SetStates)
			{
				if (state.Set == nullptr)
					continue;

				pool->ReleaseSet(state.Set);
			}
		}
	}
}
