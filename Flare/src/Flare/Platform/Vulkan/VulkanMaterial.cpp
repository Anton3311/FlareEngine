#include "VulkanMaterial.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanShader.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"

namespace Flare
{
	void VulkanMaterial::SetShader(const Ref<Shader>& shader)
	{
		if (m_Shader && m_Set)
		{
			Ref<VulkanShader> vulkanShader = As<VulkanShader>(m_Shader);
			auto pool = vulkanShader->GetDescriptorSetPool();

			if (pool)
			{
				pool->ReleaseSet(m_Set);
			}
		}

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
	}

	Ref<VulkanPipeline> VulkanMaterial::GetPipeline(const Ref<VulkanRenderPass>& renderPass)
	{
		if (m_Pipeline != nullptr && m_Pipeline->GetCompatibleRenderPass().get() == renderPass.get())
			return m_Pipeline;

		PipelineSpecifications specifications{};
		specifications.Culling = CullingMode::Back;
		specifications.Shader = m_Shader;

		specifications.DepthBiasEnabled = m_Shader->GetMetadata()->Features.DepthBiasEnabled;
		specifications.Culling = m_Shader->GetMetadata()->Features.Culling;
		specifications.DepthTest = m_Shader->GetMetadata()->Features.DepthTesting;
		specifications.DepthWrite = m_Shader->GetMetadata()->Features.DepthWrite;
		specifications.DepthFunction = m_Shader->GetMetadata()->Features.DepthFunction;
		specifications.Blending = m_Shader->GetMetadata()->Features.Blending;

		// TODO: Sould be extracted by the ShaderCompiler from shader metadata
		specifications.DepthBiasSlopeFactor = 0.1f;
		specifications.DepthBiasConstantFactor = 0.0f;

		if (m_Shader->GetMetadata()->Type == ShaderType::_2D)
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 0, 1, ShaderDataType::Float4 }, // Color
				{ 0, 2, ShaderDataType::Float2 }, // UV
				{ 0, 3, ShaderDataType::Int }, // Texture index
				{ 0, 4, ShaderDataType::Int }, // Entity index
			});
		}
		else if (m_Shader->GetMetadata()->Type == ShaderType::Surface)
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 1, 1, ShaderDataType::Float3 }, // Normal
				{ 2, 2, ShaderDataType::Float3 }, // Tangent
				{ 3, 3, ShaderDataType::Float2 }, // UV
			});
		}
		else if (m_Shader->GetMetadata()->Type == ShaderType::FullscreenQuad)
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 1, 1, ShaderDataType::Float3 }, // Normal
				{ 2, 2, ShaderDataType::Float3 }, // Tangent
				{ 3, 3, ShaderDataType::Float2 }, // UV
			});
		}
		else
		{
			FLARE_CORE_ASSERT(false);
		}
		
		m_Pipeline = CreateRef<VulkanPipeline>(specifications, renderPass);
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

			const auto& textureProperty = ReadPropertyValue<TexturePropertyValue>((uint32_t)i);
			if (textureProperty.Texture)
			{
				m_Set->WriteImage(textureProperty.Texture, property.Binding);
			}
			else if (textureProperty.FrameBuffer)
			{
				// NOTE: FrameBuffer attachment is expected to have the right layout
				m_Set->WriteImage(textureProperty.FrameBuffer, textureProperty.FrameBufferAttachmentIndex, property.Binding);
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
}
