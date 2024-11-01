#include "PCH.h"
#include "Bloom.h"

#include "FlareCore/Serialization/Serialization.h"

#include "Flare/Renderer/PostProcessing/Bloom/BloomLuminanceIsolationPass.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPassSpecifications.h"
#include "Flare/Renderer/Viewport.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Bloom);
	FLARE_SERIALIZABLE_IMPL(Bloom);

	void Bloom::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		RenderGraphTextureId bloomTexture = renderGraph.CreateTexture(viewport.GetColorTextureFormat(), "Bloom");

		RenderGraphPassSpecifications luminanceIsolationPass{};
		luminanceIsolationPass.SetDebugName("Bloom.LuminanceIsolation");
		luminanceIsolationPass.SetType(RenderGraphPassType::Graphics);
		luminanceIsolationPass.AddInput(viewport.ColorTextureId);
		luminanceIsolationPass.AddOutput(bloomTexture);

		renderGraph.AddPass(luminanceIsolationPass, Ref<BloomLuminanceIsolationPass>::New(viewport.ColorTextureId, Ref(this)));
	}

	const SerializableObjectDescriptor& Bloom::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(Bloom);
	}
}
