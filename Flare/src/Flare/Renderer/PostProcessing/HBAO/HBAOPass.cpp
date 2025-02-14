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
		RenderGraphTextureId outputTexture,
		uint32_t subPassIndex,
		float jitterAngle)
		: m_Parameters(parameters),
		m_LinearDepth(linearDepth),
		m_OutputTexture(outputTexture),
		m_SubPassIndex(subPassIndex),
		m_JitterAngle(jitterAngle)
	{
		FLARE_PROFILE_FUNCTION();

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

		Ref<Texture> outputTexture = context.GetRenderGraphResourceManager().GetTexture(m_OutputTexture);
		glm::uvec2 outputTextureSize = outputTexture->GetSize();
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
			m_ConstantBuffer.SetProperty<float>(*jitterAngle, m_JitterAngle);

		if (sampleOffset)
			m_ConstantBuffer.SetProperty<glm::ivec2>(*sampleOffset, glm::ivec2(m_SubPassIndex % 2, m_SubPassIndex / 2));

		if (outputImageSize)
			m_ConstantBuffer.SetProperty<glm::ivec2>(*outputImageSize, static_cast<glm::ivec2>(outputTextureSize));

		commandBuffer->BindComputeShader(m_Shader);
		commandBuffer->PushConstants(m_ConstantBuffer);
		commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);

		glm::uvec2 groupSize = metadata->LocalGroupSize;
		glm::uvec2 groupCount = (outputTextureSize + groupSize - glm::uvec2(1, 1)) / groupSize;
		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}

	//
	// HBAOCombineDeinterleavedTexturesPass
	//

	HBAOCombineDeinterleavedTexturesPass::HBAOCombineDeinterleavedTexturesPass(const TextureIdsArray& textures)
		: m_Textures(textures)
	{
		FLARE_PROFILE_FUNCTION();

		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("HBAOCombineDeinterleavedTextures"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}
	}

	void HBAOCombineDeinterleavedTexturesPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void HBAOCombineDeinterleavedTexturesPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_Material || !m_Material->GetShader())
		{
			FLARE_CORE_ERROR("HBAO: Invalid material or shader");
			return;
		}

		auto aoTexture0 = m_Material->GetShader()->GetPropertyIndex("u_AOTexture0");
		auto aoTexture1 = m_Material->GetShader()->GetPropertyIndex("u_AOTexture1");
		auto aoTexture2 = m_Material->GetShader()->GetPropertyIndex("u_AOTexture2");
		auto aoTexture3 = m_Material->GetShader()->GetPropertyIndex("u_AOTexture3");

		bool isValid = aoTexture0 && aoTexture1 && aoTexture2 && aoTexture3;

		if (!isValid)
		{
			FLARE_CORE_ERROR("HBAO: Shader doesn't have 'u_AOTexture' properties");
			return;
		}

		m_Material->SetTextureProperty(*aoTexture0, context.GetRenderGraph().GetTexture(m_Textures[0]));
		m_Material->SetTextureProperty(*aoTexture1, context.GetRenderGraph().GetTexture(m_Textures[1]));
		m_Material->SetTextureProperty(*aoTexture2, context.GetRenderGraph().GetTexture(m_Textures[2]));
		m_Material->SetTextureProperty(*aoTexture3, context.GetRenderGraph().GetTexture(m_Textures[3]));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
