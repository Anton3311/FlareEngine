#include "PCH.h"
#include "Bloom.h"

#include "FlareCore/Serialization/Serialization.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/PostProcessing/Bloom/BloomBlitPass.h"
#include "Flare/Renderer/PostProcessing/Bloom/BloomDownsamplePass.h"
#include "Flare/Renderer/PostProcessing/Bloom/BloomLuminanceIsolationPass.h"
#include "Flare/Renderer/PostProcessing/Bloom/BloomUpsamplePass.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPassSpecifications.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/Sampler.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Bloom);
	FLARE_SERIALIZABLE_IMPL(Bloom);

	void Bloom::RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		constexpr uint32_t MIP_COUNT = 6;

		std::vector<RenderGraphTextureId> mips;
		mips.reserve(MIP_COUNT);

		const Viewport& viewport = renderWorld.GetEntityComponent<const Viewport>(viewportEntity);
		const ViewportColorOutput& colorOutput = renderWorld.GetEntityComponent<const ViewportColorOutput>(viewportEntity);
		Ref<const Texture> viewportColorTexture = renderGraph.GetTexture(colorOutput.Id);

		glm::uvec2 textureSize = viewport.Size;
		for (uint32_t i = 0; i < MIP_COUNT; i++)
		{
			RenderGraphTextureId bloomTexture = renderGraph.GetResourceManager().CreateFixedSizeTexture(
				viewportColorTexture->GetFormat(),
				textureSize,
				fmt::format("BloomMip.{}", i));

			mips.push_back(bloomTexture);
			textureSize /= 2;
		}

		RenderGraphPassSpecifications luminanceIsolationPass{};
		luminanceIsolationPass.SetDebugName("Bloom.LuminanceIsolation");
		luminanceIsolationPass.SetType(RenderGraphPassType::Graphics);
		luminanceIsolationPass.AddInput(colorOutput.Id);
		luminanceIsolationPass.AddOutput(mips[0]);

		renderGraph.AddPass(luminanceIsolationPass, Ref<BloomLuminanceIsolationPass>::New(colorOutput.Id, Ref(this)));

		// NOTE: Clamping to the border prevents the bloom from growing in intensity closer to borders
		SamplerSpecifications samplerSpecifications{};
		samplerSpecifications.Filter = TextureFiltering::Linear;
		samplerSpecifications.WrapMode = TextureWrap::ClampToBorder;
		samplerSpecifications.BorderColor = BorderColor::FloatOpaqueBlack;

		Ref<Sampler> sampler = Sampler::Create(samplerSpecifications);

		// Downsampling Passes

		for (uint32_t i = 1; i < MIP_COUNT; i++)
		{
			RenderGraphPassSpecifications downsamplePass{};
			downsamplePass.SetDebugName(fmt::format("BloomDownsamplePass.{}", i));
			downsamplePass.SetType(RenderGraphPassType::Graphics);
			downsamplePass.AddInput(mips[i - 1]);
			downsamplePass.AddOutput(mips[i]);

			bool reduceDynamicRange = i == 1;
			renderGraph.AddPass(downsamplePass, Ref<BloomDownsamplePass>::New(mips[i - 1], sampler, reduceDynamicRange));
		}

		// Upsampling Passes

		{
			uint32_t passIndex = 1;
			for (uint32_t i = MIP_COUNT - 1; i > 0; i--, passIndex++)
			{
				RenderGraphPassSpecifications upsamplePass{};
				upsamplePass.SetDebugName(fmt::format("BloomUpsamplePass.{}", passIndex));
				upsamplePass.SetType(RenderGraphPassType::Graphics);
				upsamplePass.AddInput(mips[i]);
				upsamplePass.AddOutput(mips[i - 1]);

				renderGraph.AddPass(upsamplePass, Ref<BloomUpsamplePass>::New(Ref(this), sampler, mips[i]));
			}
		}

		RenderGraphPassSpecifications blitPass{};
		blitPass.SetDebugName("BloomBlitPass");
		blitPass.SetType(RenderGraphPassType::Graphics);
		blitPass.AddInput(mips[0]);
		blitPass.AddOutput(colorOutput.Id);

		renderGraph.AddPass(blitPass, Ref<BloomBlitPass>::New(mips[0], Ref(this)));
	}

	const SerializableObjectDescriptor& Bloom::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(Bloom);
	}
}
