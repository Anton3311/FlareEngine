#include "PCH.h"

#include "HBAOPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/Renderer/PostProcessing/SSAO.h"

namespace Flare
{
	HBAOPass::HBAOPass(Ref<SSAO> parameters,
		RenderGraphTextureId linearDepth,
		const RenderGraphTextureId* outputTextures,
		const float* jitterAngles)
		: m_Parameters(parameters),
		m_LinearDepth(linearDepth)
	{
		FLARE_PROFILE_FUNCTION();

		std::memcpy(m_JitterAngles, jitterAngles, sizeof(m_JitterAngles));
		std::memcpy(m_OutputTextures, outputTextures, sizeof(m_OutputTextures));

		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("HBAOCompute"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Shader = AssetManager::GetAsset<ComputeShader>(*shaderHandle);

			if (m_Shader)
			{
				m_ConstantBuffer.SetShader(m_Shader);
				m_DescriptorBuffer.SetShader(m_Shader);
			}
		}
	}

	void HBAOPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void HBAOPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_Shader)
			return;

		const ViewportGlobalResources* viewportResources = context.RenderWorld.TryGetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		FLARE_CORE_ASSERT(viewportResources);

		commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().CameraDescriptorSet, 0);

		Ref<const ComputeShaderMetadata> metadata = m_Shader->GetMetadata();
		std::optional<size_t> depthTextureProperty = metadata->FindDescriptorProperty("u_DepthTexture");
		std::optional<size_t> outputImageProperty = metadata->FindDescriptorProperty("u_OutputAO");

		std::optional<size_t> radiusProperty = metadata->FindConstantProperty("u_Radius");
		std::optional<size_t> radiusSquaredProperty = metadata->FindConstantProperty("u_RadiusSquared");
		std::optional<size_t> negativeInverseSquaredRadiusProperty = metadata->FindConstantProperty("u_NegativeInverseSquaredRadius");

		std::optional<size_t> biasProperty = metadata->FindConstantProperty("u_Bias");

		std::optional<size_t> depthTextureSizeProperty = metadata->FindConstantProperty("u_DepthTextureSize");
		std::optional<size_t> inverseDepthTextureSize = metadata->FindConstantProperty("u_InverseDepthTextureSize");

		std::optional<size_t> intensityProperty = metadata->FindConstantProperty("u_Intensity");
		std::optional<size_t> projectionParams = metadata->FindConstantProperty("u_InverseProjectionParams");

		std::optional<size_t> jitterAngle = metadata->FindConstantProperty("u_JitterAngle");
		std::optional<size_t> sampleOffset = metadata->FindConstantProperty("u_SampleOffset");
		std::optional<size_t> outputImageSize = metadata->FindConstantProperty("u_OutputImageSize");

		commandBuffer->BindComputeShader(m_Shader);
		for (uint32_t subPassIndex = 0; subPassIndex < 4; subPassIndex++)
		{
			Ref<Texture> outputTexture = context.GetRenderGraphResourceManager().GetTexture(m_OutputTextures[subPassIndex]);
			glm::uvec2 outputTextureSize = outputTexture->GetSize();

			Ref<Texture> depthTexture = context.GetRenderGraph().GetTexture(m_LinearDepth);

			if (depthTextureProperty)
				m_DescriptorBuffer.SetTexture(*depthTextureProperty, depthTexture);
			if (outputImageProperty)
				m_DescriptorBuffer.SetTexture(*outputImageProperty, outputTexture);

			if (radiusProperty)
				m_ConstantBuffer.SetProperty<float>(*radiusProperty, m_Parameters->Radius);

			float radiusSquared = m_Parameters->Radius * m_Parameters->Radius;
			if (radiusSquaredProperty)
				m_ConstantBuffer.SetProperty<float>(*radiusSquaredProperty, radiusSquared);

			if (negativeInverseSquaredRadiusProperty)
				m_ConstantBuffer.SetProperty<float>(*negativeInverseSquaredRadiusProperty, -1.0f / radiusSquared);

			if (biasProperty)
				m_ConstantBuffer.SetProperty<float>(*biasProperty, glm::radians(m_Parameters->Bias));

			glm::vec2 depthTextureSize = static_cast<glm::vec2>(depthTexture->GetSize());

			if (depthTextureSizeProperty)
				m_ConstantBuffer.SetProperty<glm::vec2>(*depthTextureSizeProperty, depthTextureSize);
			if (inverseDepthTextureSize)
				m_ConstantBuffer.SetProperty<glm::vec2>(*inverseDepthTextureSize, glm::vec2(1.0f) / depthTextureSize);

			if (projectionParams)
			{
				const glm::mat4& inverseProjection = context.GetRenderView().InverseProjection;
				m_ConstantBuffer.SetProperty<glm::vec2>(*projectionParams, glm::vec2(inverseProjection[0][0], inverseProjection[1][1]));
			}

			if (intensityProperty)
				m_ConstantBuffer.SetProperty<float>(*intensityProperty, m_Parameters->Intensity);

			if (jitterAngle)
				m_ConstantBuffer.SetProperty<float>(*jitterAngle, m_JitterAngles[subPassIndex]);

			if (sampleOffset)
				m_ConstantBuffer.SetProperty<glm::ivec2>(*sampleOffset, glm::ivec2(subPassIndex % 2, subPassIndex / 2));

			if (outputImageSize)
				m_ConstantBuffer.SetProperty<glm::ivec2>(*outputImageSize, static_cast<glm::ivec2>(outputTextureSize));

			commandBuffer->PushConstants(m_ConstantBuffer);
			commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);

			glm::uvec2 groupSize = metadata->LocalGroupSize;
			glm::uvec2 groupCount = (outputTextureSize + groupSize - glm::uvec2(1, 1)) / groupSize;
			commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
		}
	}

	//
	// HBAOCombineDeinterleavedTexturesPass
	//

	HBAOCombineDeinterleavedTexturesPass::HBAOCombineDeinterleavedTexturesPass(const TextureIdsArray& textures, RenderGraphTextureId outputImage)
		: m_Textures(textures), m_OutputImage(outputImage)
	{
		FLARE_PROFILE_FUNCTION();

		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("HBAOCombineDeinterleavedTextures"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_ComputeShader = AssetManager::GetAsset<ComputeShader>(*shaderHandle);

			if (m_ComputeShader)
			{
				m_DescriptorBuffer.SetShader(m_ComputeShader);
				m_ConstantBuffer.SetShader(m_ComputeShader);
			}
		}
	}

	void HBAOCombineDeinterleavedTexturesPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void HBAOCombineDeinterleavedTexturesPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_ComputeShader)
		{
			FLARE_CORE_ERROR("HBAO: Invalid material or shader");
			return;
		}

		Ref<const ComputeShaderMetadata> metadata = m_ComputeShader->GetMetadata();

		auto aoTexture0 = metadata->FindDescriptorProperty("u_AOTexture0");
		auto aoTexture1 = metadata->FindDescriptorProperty("u_AOTexture1");
		auto aoTexture2 = metadata->FindDescriptorProperty("u_AOTexture2");
		auto aoTexture3 = metadata->FindDescriptorProperty("u_AOTexture3");

		auto outputTextureProperty = metadata->FindDescriptorProperty("u_OutputImage");
		auto outputImageSizeProperty = metadata->FindConstantProperty("u_OutputImageSize");

		bool isValid = aoTexture0 && aoTexture1 && aoTexture2 && aoTexture3 && outputTextureProperty && outputImageSizeProperty;

		if (!isValid)
		{
			FLARE_CORE_ERROR("HBAO: Shader doesn't have 'u_AOTexture' properties");
			return;
		}

		Ref<Texture> outputTexture = context.GetRenderGraph().GetTexture(m_OutputImage);
		m_ConstantBuffer.SetProperty<glm::ivec2>(*outputImageSizeProperty, static_cast<glm::ivec2>(outputTexture->GetSize()));

		m_DescriptorBuffer.SetTexture(*aoTexture0, context.GetRenderGraph().GetTexture(m_Textures[0]));
		m_DescriptorBuffer.SetTexture(*aoTexture1, context.GetRenderGraph().GetTexture(m_Textures[1]));
		m_DescriptorBuffer.SetTexture(*aoTexture2, context.GetRenderGraph().GetTexture(m_Textures[2]));
		m_DescriptorBuffer.SetTexture(*aoTexture3, context.GetRenderGraph().GetTexture(m_Textures[3]));
		m_DescriptorBuffer.SetTexture(*outputTextureProperty, outputTexture);

		commandBuffer->BindComputeShader(m_ComputeShader);
		commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);
		commandBuffer->PushConstants(m_ConstantBuffer);

		// The compute shader is ran on 2x2 pixel regions
		glm::uvec2 outputImagePixelCount = (context.GetRenderGraphResourceManager().GetTexture(m_OutputImage)->GetSize() + glm::uvec2(1, 1)) / 2u;

		glm::uvec2 groupSize = metadata->LocalGroupSize;
		glm::uvec2 groupCount = (outputImagePixelCount + groupSize - glm::uvec2(1, 1)) / groupSize;
		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}
}
