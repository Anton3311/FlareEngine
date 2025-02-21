#include "PCH.h"

#include "HBAOBilateralBlurPass.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/Renderer/PostProcessing/SSAO.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	HBAOBilateralBlurPass::HBAOBilateralBlurPass(Ref<SSAO> parameters,
		bool isVertical,
		RenderGraphTextureId linearDepthTexture,
		RenderGraphTextureId aoTexture,
		RenderGraphTextureId outputTexture)
		: m_Parameters(parameters),
		m_IsVertical(isVertical),
		m_LinearDepthTexture(linearDepthTexture),
		m_AOTexture(aoTexture),
		m_OutputTexture(outputTexture)
	{
		FLARE_PROFILE_FUNCTION();

		if (auto shaderHandle = ShaderLibrary::FindShader("HBAOBilateralBlur"))
		{
			m_Shader = AssetManager::GetAsset<ComputeShader>(*shaderHandle);

			if (m_Shader)
			{
				m_ConstantBuffer.SetShader(m_Shader);
				m_DescriptorBuffer.SetShader(m_Shader);
			}
			else
			{
				FLARE_CORE_ERROR("No valid HBAOBilateralBlur shader");
			}
		}
	}

	void HBAOBilateralBlurPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void HBAOBilateralBlurPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_Shader)
			return;

		Ref<const ComputeShaderMetadata> metadata = m_Shader->GetMetadata();

		std::optional<size_t> aoProperty = metadata->FindDescriptorProperty("u_AO");
		std::optional<size_t> outputAOProperty = metadata->FindDescriptorProperty("u_OutputAO");
		std::optional<size_t> linearDepthProperty = metadata->FindDescriptorProperty("u_LinearDepth");

		std::optional<size_t> sharpnessProperty = metadata->FindConstantProperty("u_Sharpness");
		std::optional<size_t> directionProperty = metadata->FindConstantProperty("u_Direction");
		std::optional<size_t> imageSizeProperty = metadata->FindConstantProperty("u_ImageSize");

		if (sharpnessProperty)
			m_ConstantBuffer.SetProperty<float>(*sharpnessProperty, m_Parameters->Sharpness);

		if (directionProperty)
		{
			glm::ivec2 direction = m_IsVertical ? glm::ivec2(0, 1) : glm::ivec2(1, 0);
			m_ConstantBuffer.SetProperty<glm::ivec2>(*directionProperty, direction);
		}
		
		glm::uvec2 outputTextureSize = context.GetRenderGraphResourceManager().GetTexture(m_AOTexture)->GetSize();
		if (imageSizeProperty)
		{
			m_ConstantBuffer.SetProperty<glm::ivec2>(*imageSizeProperty, static_cast<glm::ivec2>(outputTextureSize));
		}

		if (aoProperty)
		{
			m_DescriptorBuffer.SetTexture(*aoProperty, context.GetRenderGraphResourceManager().GetTexture(m_AOTexture));
		}

		if (outputAOProperty)
		{
			m_DescriptorBuffer.SetTexture(*outputAOProperty, context.GetRenderGraphResourceManager().GetTexture(m_OutputTexture));
		}

		if (linearDepthProperty)
		{
			m_DescriptorBuffer.SetTexture(*linearDepthProperty, context.GetRenderGraphResourceManager().GetTexture(m_LinearDepthTexture));
		}

		glm::uvec2 groupSize = metadata->LocalGroupSize;
		glm::uvec2 groupCount = (outputTextureSize + groupSize - glm::uvec2(1, 1)) / groupSize;

		commandBuffer->BindComputeShader(m_Shader);
		commandBuffer->PushConstants(m_ConstantBuffer);
		commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);
		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}
}
