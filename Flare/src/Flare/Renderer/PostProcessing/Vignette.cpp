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

	Vignette::Vignette()
		: Enabled(false), Color(0.0f, 0.0f, 0.0f, 0.5f), Radius(1.0f), Smoothness(1.0f)
	{
		m_Shader = Shader::Create("assets/Shaders/Vignette.glsl");
	}

	void Vignette::OnRender(RenderingContext& context)
	{
		if (!Enabled)
			return;

		m_Shader->Bind();
		m_Shader->SetFloat4("u_Params.Color", Color);
		m_Shader->SetFloat("u_Params.Radius", Radius);
		m_Shader->SetFloat("u_Params.Smoothness", Smoothness);

		RenderCommand::DrawIndexed(Renderer::GetFullscreenQuad());
	}
}
