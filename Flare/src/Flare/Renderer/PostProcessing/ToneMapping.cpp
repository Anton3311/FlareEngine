#include "Tonemapping.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderCommand.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ToneMapping, FLARE_FIELD(ToneMapping, Enabled));

	ToneMapping::ToneMapping()
		: Enabled(false)
	{
		m_Shader = Shader::Create("assets/Shaders/AcesToneMapping.glsl");
	}

	void ToneMapping::OnRender(RenderingContext& context)
	{
		if (!Enabled)
			return;

		Ref<FrameBuffer> output = context.RTPool.Get();

		context.RenderTarget->BindAttachmentTexture(0, 0);
		output->Bind();

		m_Shader->Bind();
		RenderCommand::DrawIndexed(Renderer::GetFullscreenQuad());

		context.RenderTarget->Blit(output, 0, 0);
		context.RTPool.Release(output);
		context.RenderTarget->Bind();
	}
}
