#include "SSAO.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Material.h"

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

	void SSAO::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		RegisterHBAORenderPasses(renderGraph, viewport);
	}

	const SerializableObjectDescriptor& SSAO::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(SSAO);
	}

	void SSAO::RegisterHBAORenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		RenderGraphTextureId linearDepthDepth = renderGraph.CreateTexture(TextureFormat::RF32, "HBAO.DownsampledDepth");
		RenderGraphTextureId aoTexture = renderGraph.CreateTexture(TextureFormat::R8, "HBAO.AO");
		RenderGraphTextureId aoBlurIntermediateTexture = renderGraph.CreateTexture(TextureFormat::R8, "HBAO.AOBlurIntermediate");

		RenderGraphPassSpecifications downsamplePass{};
		downsamplePass.SetDebugName("HBAODownsamplePass");
		downsamplePass.SetType(RenderGraphPassType::Graphics);
		downsamplePass.AddInput(viewport.DepthTextureId);
		downsamplePass.AddOutput(linearDepthDepth, 0);

		renderGraph.AddPass(downsamplePass, Ref<HBAODownsamplePass>::New(viewport.DepthTextureId));

		RenderGraphPassSpecifications aoPass{};
		aoPass.SetDebugName("HBAOPass");
		aoPass.SetType(RenderGraphPassType::Graphics);
		aoPass.AddInput(linearDepthDepth);
		aoPass.AddOutput(aoTexture, 0);
	
		renderGraph.AddPass(aoPass, Ref<HBAOPass>::New(Ref<SSAO>(this), linearDepthDepth));

		{
			RenderGraphPassSpecifications verticalBlurPass{};
			verticalBlurPass.AddInput(aoTexture);
			verticalBlurPass.AddOutput(aoBlurIntermediateTexture, 0, glm::vec4(0.0f));
			verticalBlurPass.SetType(RenderGraphPassType::Graphics);
			verticalBlurPass.SetDebugName("HBAO Vertical Bilateral Blur");

			renderGraph.AddPass(verticalBlurPass, Ref<HBAOBilateralBlurPass>::New(Ref<SSAO>(this), true, linearDepthDepth, aoTexture));
		}

		{
			RenderGraphPassSpecifications horizontalBlurPass{};
			horizontalBlurPass.AddInput(aoBlurIntermediateTexture);
			horizontalBlurPass.AddOutput(aoTexture, 0, glm::vec4(0.0f));
			horizontalBlurPass.SetType(RenderGraphPassType::Graphics);
			horizontalBlurPass.SetDebugName("HBAO Horizontal Bilateral Blur");

			renderGraph.AddPass(horizontalBlurPass, Ref<HBAOBilateralBlurPass>::New(Ref<SSAO>(this), false, linearDepthDepth, aoBlurIntermediateTexture));
		}

		RenderGraphPassSpecifications ssaoComposingPass{};
		ssaoComposingPass.SetDebugName("SSAOComposingPass");
		ssaoComposingPass.SetType(RenderGraphPassType::Compute);
		ssaoComposingPass.AddInput(aoTexture);
		ssaoComposingPass.AddResource(viewport.ColorTextureId, ResourceAccess::ReadWrite);

		renderGraph.AddPass(ssaoComposingPass, Ref<SSAOComposingPass>::New(viewport.ColorTextureId, aoTexture));
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

		glm::uvec2 renderAreaSize = (glm::uvec2)context.GetViewport().GetSize();
		glm::uvec2 localGroupSize = m_Shader->GetMetadata()->LocalGroupSize;

		glm::uvec2 groupCount = (renderAreaSize + localGroupSize - glm::uvec2(1)) / localGroupSize;

		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}
}
