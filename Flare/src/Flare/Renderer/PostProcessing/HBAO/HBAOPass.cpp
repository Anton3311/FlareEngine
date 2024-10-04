#include "HBAOPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Renderer/PostProcessing/SSAO.h"

namespace Flare
{
	HBAOPass::HBAOPass(Ref<SSAO> parameters, RenderGraphTextureId downsampledDepth)
		: m_Parameters(parameters), m_DownsampledDepth(downsampledDepth)
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

		commandBuffer->SetDefaltViewportAndScissors();

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);

		std::optional<uint32_t> depthTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");
		std::optional<uint32_t> radiusProperty = m_Material->GetShader()->GetPropertyIndex("u_Radius");
		std::optional<uint32_t> tangentBiasProperty = m_Material->GetShader()->GetPropertyIndex("u_TangentBias");
		std::optional<uint32_t> depthTextureSizeProperty = m_Material->GetShader()->GetPropertyIndex("u_DepthTextureSize");
		std::optional<uint32_t> intensityProperty = m_Material->GetShader()->GetPropertyIndex("u_Intensity");
		std::optional<uint32_t> projectionParams = m_Material->GetShader()->GetPropertyIndex("u_InverseProjectionParams");

		Ref<Texture> depthTexture = context.GetRenderGraph().GetTexture(m_DownsampledDepth);

		if (depthTextureProperty)
			m_Material->SetTextureProperty(*depthTextureProperty, depthTexture);

		if (radiusProperty)
			m_Material->WritePropertyValue<float>(*radiusProperty, m_Parameters->Radius);

		if (tangentBiasProperty)
			m_Material->WritePropertyValue<float>(*tangentBiasProperty, glm::radians(m_Parameters->Bias));

		if (depthTextureSizeProperty)
		{
			const TextureSpecifications& specifications = depthTexture->GetSpecifications();
			m_Material->WritePropertyValue<glm::vec2>(*depthTextureSizeProperty, (glm::vec2)glm::uvec2(specifications.Width, specifications.Height));
		}

		if (projectionParams)
		{
			const glm::mat4& inverseProjection = context.GetRenderView().InverseProjection;
			m_Material->WritePropertyValue<glm::vec2>(*projectionParams, glm::vec2(inverseProjection[0][0], inverseProjection[1][1]));
		}

		if (intensityProperty)
			m_Material->WritePropertyValue<float>(*intensityProperty, m_Parameters->Intensity);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
