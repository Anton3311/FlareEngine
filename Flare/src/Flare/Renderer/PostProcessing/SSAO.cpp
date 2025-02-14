#include "PCH.h"

#include "SSAO.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/Renderer/Passes/BlitPass.h"

#include "Flare/Renderer/PostProcessing/HBAO/HBAOBilateralBlurPass.h"
#include "Flare/Renderer/PostProcessing/HBAO/HBAODownsamplePass.h"
#include "Flare/Renderer/PostProcessing/HBAO/HBAOPass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include <random>

namespace Flare
{
	FLARE_IMPL_TYPE(SSAO);
	FLARE_SERIALIZABLE_IMPL(SSAO);

	SSAO::SSAO()
		: PostProcessingEffect(PostProcessingExecutionOrder::AfterDepthPrePass)
	{
	}

	void SSAO::RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		RegisterHBAORenderPasses(renderGraph, viewportEntity, renderWorld);
	}

	const SerializableObjectDescriptor& SSAO::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(SSAO);
	}

	void SSAO::RegisterHBAORenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();

		// FIXME: Should allow accepting a mutable reference to the World, instead of getting it from the Renderer
		AOConfiguration& aoConfiguration = Renderer::GetRenderWorld().GetEntityComponent<AOConfiguration>(viewportEntity);

		constexpr TextureFormat AO_TEXTURE_FORMAT = TextureFormat::R8;

		std::default_random_engine engine;
		std::uniform_real_distribution<float> generator(0.0f, 2.0f * glm::pi<float>());

		RenderGraphTextureId linearDepthTexture = renderGraph.CreateTexture(TextureFormat::RF32, "HBAO.LinearDepth");
		RenderGraphTextureId aoBlurIntermediateTexture = renderGraph.CreateTexture(AO_TEXTURE_FORMAT, "HBAO.AOBlurIntermediate");

		// FIXME: Switch back to on demand allocation.
		//       It isn't possible at the moment as the AO texture in global descriptor sets is not updated every frame.
		//
		//       When updating descriptor sets, all sets for each frame in flight might get filled with the same AO texture,
		//       because not all of the textures had been allocated at this point.
		//       This leads to validation errors, because all frames in flight use the same texture.
		//       To work around this, the texture resource is created with `RenderGraphTextureAllocationMode::Preallocated`.
		RenderGraphTextureId fullScreenAOTexture = renderGraph.GetResourceManager().CreateTexture(AO_TEXTURE_FORMAT,
			"HBAO.FullScreenAO",
			1.0f, RenderGraphTextureAllocationMode::Preallocated);

		aoConfiguration.AOTexture = fullScreenAOTexture;

		constexpr size_t TEXTURE_COUNT = 4;

		std::array<RenderGraphTextureId, TEXTURE_COUNT> aoTextures;
		for (size_t i = 0; i < TEXTURE_COUNT; i++)
		{
			aoTextures[i] = renderGraph.CreateTexture(TextureFormat::R8, fmt::format("HBAO.AO.{}", i), 0.5f);
		}

		RenderGraphTextureId viewportDepthTexture = renderWorld.GetEntityComponent<const ViewportDepthOutput>(viewportEntity).Id;
		RenderGraphTextureId viewportColorTexture = renderWorld.GetEntityComponent<const ViewportColorOutput>(viewportEntity).Id;

		RenderGraphPassSpecifications linearizeDepthPass{};
		linearizeDepthPass.SetDebugName("HBAOLinearizeDepth");
		linearizeDepthPass.SetType(RenderGraphPassType::Compute);
		linearizeDepthPass.AddInput(viewportDepthTexture);
		linearizeDepthPass.AddResource(linearDepthTexture, ResourceAccess::Write);

		renderGraph.AddPass(linearizeDepthPass, Ref<HBAOLinearizeDepthPass>::New(viewportDepthTexture, linearDepthTexture));

		RenderGraphPassSpecifications aoPass{};
		aoPass.SetDebugName("HBAOPass");
		aoPass.SetType(RenderGraphPassType::Compute);
		aoPass.AddInput(linearDepthTexture);

		for (RenderGraphTextureId aoTexture : aoTextures)
			aoPass.AddResource(aoTexture, ResourceAccess::Write);

		float jitterAngels[4];
		for (uint32_t i = 0; i < 4; i++)
			jitterAngels[i] = generator(engine);

		renderGraph.AddPass(aoPass, Ref<HBAOPass>::New(Ref<SSAO>(this), linearDepthTexture, aoTextures.data(), jitterAngels));

		RenderGraphPassSpecifications combinePass{};
		combinePass.SetDebugName("HBAOCombineDeinterleavedTexturesPass");
		combinePass.SetType(RenderGraphPassType::Graphics);
		combinePass.AddOutput(fullScreenAOTexture);

		for (size_t i = 0; i < TEXTURE_COUNT; i++)
			combinePass.AddInput(aoTextures[i]);

		renderGraph.AddPass(combinePass, Ref<HBAOCombineDeinterleavedTexturesPass>::New(aoTextures));

		{
			RenderGraphPassSpecifications verticalBlurPass{};
			verticalBlurPass.AddInput(fullScreenAOTexture);
			verticalBlurPass.AddOutput(aoBlurIntermediateTexture, glm::vec4(0.0f));
			verticalBlurPass.SetType(RenderGraphPassType::Graphics);
			verticalBlurPass.SetDebugName("HBAO Vertical Bilateral Blur");

			renderGraph.AddPass(verticalBlurPass, Ref<HBAOBilateralBlurPass>::New(Ref<SSAO>(this), true, linearDepthTexture, fullScreenAOTexture));
		}

		{
			RenderGraphPassSpecifications horizontalBlurPass{};
			horizontalBlurPass.AddInput(aoBlurIntermediateTexture);
			horizontalBlurPass.AddOutput(fullScreenAOTexture, glm::vec4(0.0f));
			horizontalBlurPass.SetType(RenderGraphPassType::Graphics);
			horizontalBlurPass.SetDebugName("HBAO Horizontal Bilateral Blur");

			renderGraph.AddPass(horizontalBlurPass, Ref<HBAOBilateralBlurPass>::New(Ref<SSAO>(this), false, linearDepthTexture, aoBlurIntermediateTexture));
		}
	}
}
