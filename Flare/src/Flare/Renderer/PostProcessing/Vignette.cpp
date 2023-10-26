#include "Vignette.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderCommand.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Vignette,
		FLARE_FIELD(Vignette, Enabled),
		FLARE_FIELD(Vignette, Color),
		FLARE_FIELD(Vignette, Radius),
		FLARE_FIELD(Vignette, Smoothness),
	);

	static uint32_t s_ColorParameterIndex = UINT32_MAX;
	static uint32_t s_RadiusParameterIndex = UINT32_MAX;
	static uint32_t s_SmoothnessParameterIndex = UINT32_MAX;

	Vignette::Vignette()
		: Enabled(false), Color(0.0f, 0.0f, 0.0f, 0.5f), Radius(1.0f), Smoothness(1.0f)
	{
		Ref<Shader> shader = Shader::Create("assets/Shaders/Vignette.glsl");
		m_Material = CreateRef<Material>(shader);
		m_Material->Features.DepthTesting = false;

		s_ColorParameterIndex = shader->GetParameterIndex("u_Params.Color").value_or(UINT32_MAX);
		s_RadiusParameterIndex = shader->GetParameterIndex("u_Params.Radius").value_or(UINT32_MAX);
		s_SmoothnessParameterIndex = shader->GetParameterIndex("u_Params.Smoothness").value_or(UINT32_MAX);
	}

	void Vignette::OnRender(RenderingContext& context)
	{
		if (!Enabled)
			return;

		FrameBufferAttachmentsMask writeMask = context.RenderTarget->GetWriteMask();

		m_Material->WriteParameterValue(s_ColorParameterIndex, Color);
		m_Material->WriteParameterValue(s_RadiusParameterIndex, Radius);
		m_Material->WriteParameterValue(s_SmoothnessParameterIndex, Smoothness);

		context.RenderTarget->SetWriteMask(0x1);
		Renderer::DrawFullscreenQuad(m_Material);
		context.RenderTarget->SetWriteMask(writeMask);
	}
}
