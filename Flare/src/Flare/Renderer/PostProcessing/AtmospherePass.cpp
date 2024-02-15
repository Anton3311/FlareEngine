#include "AtmospherePass.h"

#include "Flare/Renderer/Renderer.h"

namespace Flare
{
	FLARE_IMPL_TYPE(AtmospherePass);

	void AtmospherePass::OnRender(RenderingContext& context)
	{
		if (!AtmosphereMaterial || !Enabled || !Renderer::GetCurrentViewport().PostProcessingEnabled)
			return;

		Renderer::DrawFullscreenQuad(AtmosphereMaterial);
	}
}
