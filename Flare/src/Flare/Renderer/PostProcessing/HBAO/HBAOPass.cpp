#include "PCH.h"

#include "HBAOPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"

#include "Flare/AssetManager/AssetManager.h"

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
		RenderGraphTextureId downsampledDepth,
		uint32_t subPassIndex,
		float jitterAngle)
		: m_Parameters(parameters), m_DownsampledDepth(downsampledDepth), m_SubPassIndex(subPassIndex), m_JitterAngle(jitterAngle)
	{
		FLARE_PROFILE_FUNCTION();

		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("HBAO"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}
	}

	void HBAOPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void HBAOPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->SetDefaultViewportAndScissors();

		const ViewportGlobalResources* viewportResources = context.RenderWorld.TryGetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		FLARE_CORE_ASSERT(viewportResources);

		commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().CameraDescriptorSet, 0);

		std::optional<uint32_t> depthTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");

		std::optional<uint32_t> radiusProperty = m_Material->GetShader()->GetPropertyIndex("u_Radius");
		std::optional<uint32_t> radiusSquaredProperty = m_Material->GetShader()->GetPropertyIndex("u_RadiusSquared");
		std::optional<uint32_t> negativeInverseSquaredRadiusProperty = m_Material->GetShader()->GetPropertyIndex("u_NegativeInverseSquaredRadius");

		std::optional<uint32_t> biasProperty = m_Material->GetShader()->GetPropertyIndex("u_Bias");

		std::optional<uint32_t> depthTextureSizeProperty = m_Material->GetShader()->GetPropertyIndex("u_DepthTextureSize");
		std::optional<uint32_t> inverseDepthTextureSize = m_Material->GetShader()->GetPropertyIndex("u_InverseDepthTextureSize");

		std::optional<uint32_t> intensityProperty = m_Material->GetShader()->GetPropertyIndex("u_Intensity");
		std::optional<uint32_t> projectionParams = m_Material->GetShader()->GetPropertyIndex("u_InverseProjectionParams");

		std::optional<uint32_t> jitterAngle = m_Material->GetShader()->GetPropertyIndex("u_JitterAngle");
		std::optional<uint32_t> sampleOffset = m_Material->GetShader()->GetPropertyIndex("u_SampleOffset");

		Ref<Texture> depthTexture = context.GetRenderGraph().GetTexture(m_DownsampledDepth);

		if (depthTextureProperty)
			m_Material->SetTextureProperty(*depthTextureProperty, depthTexture);

		if (radiusProperty)
			m_Material->WritePropertyValue<float>(*radiusProperty, m_Parameters->Radius);

		float radiusSquared = m_Parameters->Radius * m_Parameters->Radius;
		if (radiusSquaredProperty)
			m_Material->WritePropertyValue<float>(*radiusSquaredProperty, radiusSquared);

		if (negativeInverseSquaredRadiusProperty)
			m_Material->WritePropertyValue<float>(*negativeInverseSquaredRadiusProperty, -1.0f / radiusSquared);

		if (biasProperty)
			m_Material->WritePropertyValue<float>(*biasProperty, glm::radians(m_Parameters->Bias));

		const TextureSpecifications& specifications = depthTexture->GetSpecifications();
		glm::vec2 depthTextureSize = (glm::vec2)glm::uvec2(specifications.Width, specifications.Height);
		
		if (depthTextureSizeProperty)
			m_Material->WritePropertyValue<glm::vec2>(*depthTextureSizeProperty, depthTextureSize);
		if (inverseDepthTextureSize)
			m_Material->WritePropertyValue<glm::vec2>(*inverseDepthTextureSize, glm::vec2(1.0f) / depthTextureSize);

		if (projectionParams)
		{
			const glm::mat4& inverseProjection = context.GetRenderView().InverseProjection;
			m_Material->WritePropertyValue<glm::vec2>(*projectionParams, glm::vec2(inverseProjection[0][0], inverseProjection[1][1]));
		}

		if (intensityProperty)
			m_Material->WritePropertyValue<float>(*intensityProperty, m_Parameters->Intensity);

		if (jitterAngle)
			m_Material->WritePropertyValue<float>(*jitterAngle, m_JitterAngle);

		if (sampleOffset)
			m_Material->WritePropertyValue<glm::ivec2>(*sampleOffset, glm::ivec2(m_SubPassIndex % 2, m_SubPassIndex / 2));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
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
