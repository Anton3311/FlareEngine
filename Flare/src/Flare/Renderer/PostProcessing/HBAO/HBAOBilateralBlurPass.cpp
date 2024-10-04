#include "HBAOBilateralBlurPass.h"

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
		RenderGraphTextureId aoTexture)
		: m_Parameters(parameters), m_IsVertical(isVertical), m_LinearDepthTexture(linearDepthTexture), m_AOTexture(aoTexture)
	{
		FLARE_PROFILE_FUNCTION();

		if (auto shaderHandle = ShaderLibrary::FindShader("HBAOBilateralBlur"))
		{
			m_Material = Material::Create(*shaderHandle);
		}
	}

	void HBAOBilateralBlurPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void HBAOBilateralBlurPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		std::optional<uint32_t> aoProperty = m_Material->GetShader()->GetPropertyIndex("u_AO");
		std::optional<uint32_t> linearDepthProperty = m_Material->GetShader()->GetPropertyIndex("u_LinearDepth");

		std::optional<uint32_t> sharpnessProperty = m_Material->GetShader()->GetPropertyIndex("u_Sharpness");
		std::optional<uint32_t> directionProperty = m_Material->GetShader()->GetPropertyIndex("u_Direction");

		if (sharpnessProperty)
		{
			m_Material->WritePropertyValue<float>(*sharpnessProperty, m_Parameters->Sharpness);
		}

		if (directionProperty)
		{
			glm::vec2 direction = m_IsVertical ? glm::vec2(0.0f, 1.0f) : glm::vec2(1.0f, 0.0f);
			m_Material->WritePropertyValue<glm::vec2>(*directionProperty, direction / (glm::vec2)context.RenderAreaSize);
		}

		if (aoProperty)
		{
			m_Material->SetTextureProperty(*aoProperty, context.GetRenderGraphResourceManager().GetTexture(m_AOTexture));
		}

		if (linearDepthProperty)
		{
			m_Material->SetTextureProperty(*linearDepthProperty, context.GetRenderGraphResourceManager().GetTexture(m_LinearDepthTexture));
		}

		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
