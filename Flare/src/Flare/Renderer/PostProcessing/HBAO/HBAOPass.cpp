#include "HBAOPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Renderer/PostProcessing/SSAO.h"

namespace Flare
{
	HBAOPass::HBAOPass(Ref<SSAO> parameters, RenderGraphTextureId normalTexture, RenderGraphTextureId downsampledDepth)
		: m_Parameters(parameters), m_DownsampledDepth(downsampledDepth), m_NormalTexture(normalTexture)
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
		std::optional<uint32_t> normalTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_NormalTexture");
		std::optional<uint32_t> radiusProperty = m_Material->GetShader()->GetPropertyIndex("u_Radius");
		std::optional<uint32_t> tangentBiasProperty = m_Material->GetShader()->GetPropertyIndex("u_TangentBias");
		std::optional<uint32_t> depthTextureSizeProperty = m_Material->GetShader()->GetPropertyIndex("u_DepthTextureSize");
		std::optional<uint32_t> intensityProperty = m_Material->GetShader()->GetPropertyIndex("u_Intensity");
		std::optional<uint32_t> rotationOffsetProperty = m_Material->GetShader()->GetPropertyIndex("u_RotationOffset");
		std::optional<uint32_t> debugProperty = m_Material->GetShader()->GetPropertyIndex("u_Debug");

		Ref<Texture> depthTexture = context.GetRenderGraph().GetTexture(m_DownsampledDepth);

		if (depthTextureProperty)
			m_Material->SetTextureProperty(*depthTextureProperty, depthTexture);

		if (normalTextureProperty)
			m_Material->SetTextureProperty(*normalTextureProperty, context.GetRenderGraphResourceManager().GetTexture(m_NormalTexture));

		if (radiusProperty)
			m_Material->WritePropertyValue<float>(*radiusProperty, m_Parameters->Radius);

		if (tangentBiasProperty)
			m_Material->WritePropertyValue<float>(*tangentBiasProperty, glm::radians(m_Parameters->Bias));

		if (depthTextureSizeProperty)
		{
			const TextureSpecifications& specifications = depthTexture->GetSpecifications();
			m_Material->WritePropertyValue<glm::vec2>(*depthTextureSizeProperty, (glm::vec2)glm::uvec2(specifications.Width, specifications.Height));
		}

		if (debugProperty)
			m_Material->WritePropertyValue<int>(*debugProperty, m_Parameters->Debug);

		if (rotationOffsetProperty)
			m_Material->WritePropertyValue<float>(*rotationOffsetProperty, m_Parameters->RotationOffset);

		if (intensityProperty)
			m_Material->WritePropertyValue<float>(*intensityProperty, m_Parameters->Intensity);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
