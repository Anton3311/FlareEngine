#include "PCH.h"

#include "SSAO.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/FrameBuffer.h"
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

		constexpr TextureFormat AO_TEXTURE_FORMAT = TextureFormat::R8;

		std::default_random_engine engine;
		std::uniform_real_distribution<float> generator(0.0f, 2.0f * glm::pi<float>());

		RenderGraphTextureId linearDepthDepth = renderGraph.CreateTexture(TextureFormat::RF32, "HBAO.DownsampledDepth");
		RenderGraphTextureId fullScreenAOTexture = renderGraph.CreateTexture(AO_TEXTURE_FORMAT, "HBAO.FullScreenAO");
		RenderGraphTextureId aoBlurIntermediateTexture = renderGraph.CreateTexture(AO_TEXTURE_FORMAT, "HBAO.AOBlurIntermediate");

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
		linearizeDepthPass.SetType(RenderGraphPassType::Graphics);
		linearizeDepthPass.AddInput(viewportDepthTexture);
		linearizeDepthPass.AddOutput(linearDepthDepth);

		renderGraph.AddPass(linearizeDepthPass, Ref<HBAODownsamplePass>::New(viewportDepthTexture));

		for (size_t i = 0; i < TEXTURE_COUNT; i++)
		{
			RenderGraphPassSpecifications aoPass{};
			aoPass.SetDebugName("HBAOPass");
			aoPass.SetType(RenderGraphPassType::Graphics);
			aoPass.AddInput(linearDepthDepth);
			aoPass.AddOutput(aoTextures[i]);

			renderGraph.AddPass(aoPass, Ref<HBAOPass>::New(Ref<SSAO>(this), linearDepthDepth, (uint32_t)i, generator(engine)));
		}

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

			renderGraph.AddPass(verticalBlurPass, Ref<HBAOBilateralBlurPass>::New(Ref<SSAO>(this), true, linearDepthDepth, fullScreenAOTexture));
		}

		{
			RenderGraphPassSpecifications horizontalBlurPass{};
			horizontalBlurPass.AddInput(aoBlurIntermediateTexture);
			horizontalBlurPass.AddOutput(fullScreenAOTexture, glm::vec4(0.0f));
			horizontalBlurPass.SetType(RenderGraphPassType::Graphics);
			horizontalBlurPass.SetDebugName("HBAO Horizontal Bilateral Blur");

			renderGraph.AddPass(horizontalBlurPass, Ref<HBAOBilateralBlurPass>::New(Ref<SSAO>(this), false, linearDepthDepth, aoBlurIntermediateTexture));
		}

		RenderGraphPassSpecifications ssaoComposingPass{};
		ssaoComposingPass.SetDebugName("SSAOComposingPass");
		ssaoComposingPass.SetType(RenderGraphPassType::Compute);
		ssaoComposingPass.AddInput(fullScreenAOTexture);
		ssaoComposingPass.AddResource(viewportColorTexture, ResourceAccess::ReadWrite);

		renderGraph.AddPass(ssaoComposingPass, Ref<SSAOComposingPass>::New(viewportColorTexture, fullScreenAOTexture));
	}



	SSAOComposingPass::SSAOComposingPass(RenderGraphTextureId colorTexture, RenderGraphTextureId aoTexture)
		: m_ColorTexture(colorTexture), m_AOTexture(aoTexture)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAOCompose");
		if (shaderHandle && AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			m_Shader = AssetManager::GetAsset<ComputeShader>(*shaderHandle);

			Ref<const ComputeShaderMetadata> metadata = m_Shader->GetMetadata();

			m_ColorImageProperty = metadata->FindDescriptorProperty("u_Color");
			m_AOImageProperty = metadata->FindDescriptorProperty("u_AO");
			m_ImageSizeProperty = metadata->FindConstantProperty("u_ImageSize");

			m_ConstantBuffer.SetShader(m_Shader);
			m_DescriptorBuffer.SetShader(m_Shader);
		}
		else
		{
			FLARE_CORE_ERROR("SSAO: Failed to find SSAOCompose shader");
		}

		auto result = Scene::GetActive()->GetPostProcessingManager().GetEffect<SSAO>();
		FLARE_CORE_ASSERT(result.has_value());
		m_Parameters = *result;
	}

	void SSAOComposingPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const TextureSpecifications& colorTextureSpec = context.GetRenderGraphResourceManager().GetTexture(m_ColorTexture)->GetSpecifications();
		m_ConstantBuffer.SetProperty(*m_ImageSizeProperty, glm::ivec2((int32_t)colorTextureSpec.Width, (int32_t)colorTextureSpec.Height));

		m_DescriptorBuffer.SetTexture(*m_AOImageProperty, context.GetRenderGraphResourceManager().GetTexture(m_AOTexture));
		m_DescriptorBuffer.SetTexture(*m_ColorImageProperty, context.GetRenderGraphResourceManager().GetTexture(m_ColorTexture));
	}

	void SSAOComposingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->BindComputeShader(m_Shader);
		commandBuffer->PushConstants(m_ConstantBuffer);
		commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);

		const Viewport& viewport = context.RenderWorld.GetEntityComponent<const Viewport>(context.ViewportEntity);

		glm::uvec2 renderAreaSize = viewport.Size;
		glm::uvec2 localGroupSize = m_Shader->GetMetadata()->LocalGroupSize;

		glm::uvec2 groupCount = (renderAreaSize + localGroupSize - glm::uvec2(1)) / localGroupSize;

		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}
}
