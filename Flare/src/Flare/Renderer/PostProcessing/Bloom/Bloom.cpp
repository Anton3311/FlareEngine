#include "PCH.h"
#include "Bloom.h"

#include "FlareCore/Serialization/Serialization.h"

#include "Flare/Renderer/PostProcessing/Bloom/BloomFilteringPass.h"
#include "Flare/Renderer/PostProcessing/Bloom/BloomLuminanceIsolationPass.h"
#include "Flare/Renderer/PostProcessing/Bloom/BloomBlitPass.h"
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

		uint32_t mipCount = CalculateMipCount(viewport.GetSize().x, viewport.GetSize().y) - 4;
		FLARE_CORE_ASSERT(mipCount > 0);

		std::vector<RenderGraphTextureId> mips;
		mips.reserve(mipCount);

		glm::uvec2 textureSize = viewport.GetSize();
		for (uint32_t i = 0; i < mipCount; i++)
		{
			RenderGraphTextureId bloomTexture = renderGraph.GetResourceManager().CreateFixedSizeTexture(
				viewport.GetColorTextureFormat(),
				textureSize,
				fmt::format("BloomMip.{}", i));

			mips.push_back(bloomTexture);
			textureSize /= 2;
		}

		RenderGraphPassSpecifications luminanceIsolationPass{};
		luminanceIsolationPass.SetDebugName("Bloom.LuminanceIsolation");
		luminanceIsolationPass.SetType(RenderGraphPassType::Graphics);
		luminanceIsolationPass.AddInput(viewport.ColorTextureId);
		luminanceIsolationPass.AddOutput(mips[0]);

		renderGraph.AddPass(luminanceIsolationPass, Ref<BloomLuminanceIsolationPass>::New(viewport.ColorTextureId, Ref(this)));

		// Downsampling Passes
		for (uint32_t i = 1; i < mipCount; i++)
		{
			RenderGraphPassSpecifications downsamplePass{};
			downsamplePass.SetDebugName(fmt::format("BloomDownsamplePass.{}", i));
			downsamplePass.SetType(RenderGraphPassType::Graphics);
			downsamplePass.AddInput(mips[i - 1]);
			downsamplePass.AddOutput(mips[i]);

			renderGraph.AddPass(downsamplePass, Ref<BloomFilteringPass>::New(mips[i - 1]));
		}

		// Upsampling Passes
		for (uint32_t i = mipCount - 1; i > 0; i--)
		{
			RenderGraphPassSpecifications upsamplePass{};
			upsamplePass.SetDebugName(fmt::format("BloomUpsamplePass.{}", i));
			upsamplePass.SetType(RenderGraphPassType::Graphics);
			upsamplePass.AddInput(mips[i]);
			upsamplePass.AddOutput(mips[i - 1]);

			renderGraph.AddPass(upsamplePass, Ref<BloomFilteringPass>::New(mips[i]));
		}

		RenderGraphPassSpecifications blitPass{};
		blitPass.SetDebugName("BloomBlitPass");
		blitPass.SetType(RenderGraphPassType::Graphics);
		blitPass.AddInput(mips[0]);
		blitPass.AddOutput(viewport.ColorTextureId);

		renderGraph.AddPass(blitPass, Ref<BloomBlitPass>::New(mips[0], Ref(this)));
	}

	const SerializableObjectDescriptor& Bloom::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(Bloom);
	}
}
