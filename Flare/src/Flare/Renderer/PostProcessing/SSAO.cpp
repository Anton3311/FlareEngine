#include "SSAO.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Passes/BlitPass.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include <random>

namespace Flare
{
	FLARE_IMPL_TYPE(SSAO);

	SSAO::SSAO()
		: Bias(0.1f), Radius(0.5f), BlurSize(2.0f)
	{
	}

	void SSAO::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		if (!IsEnabled())
			return;

		TextureSpecifications aoTextureSpecifications{};
		aoTextureSpecifications.Filtering = TextureFiltering::Closest;
		aoTextureSpecifications.Format = TextureFormat::RF32;
		aoTextureSpecifications.Wrap = TextureWrap::Clamp;
		aoTextureSpecifications.GenerateMipMaps = false;
		aoTextureSpecifications.Width = viewport.GetSize().x;
		aoTextureSpecifications.Height = viewport.GetSize().y;
		aoTextureSpecifications.Usage = TextureUsage::RenderTarget | TextureUsage::Sampling;

		TextureSpecifications colorTextureSpecifications{};
		colorTextureSpecifications.Filtering = TextureFiltering::Closest;
		colorTextureSpecifications.Format = TextureFormat::R11G11B10;
		colorTextureSpecifications.Wrap = TextureWrap::Clamp;
		colorTextureSpecifications.GenerateMipMaps = false;
		colorTextureSpecifications.Width = viewport.GetSize().x;
		colorTextureSpecifications.Height = viewport.GetSize().y;
		colorTextureSpecifications.Usage = TextureUsage::RenderTarget | TextureUsage::Sampling;

		Ref<Texture> aoTexture = Texture::Create(aoTextureSpecifications);
		Ref<Texture> intermediateColorTexture = Texture::Create(colorTextureSpecifications);

		aoTexture->SetDebugName("SSAO.AOTexture");
		intermediateColorTexture->SetDebugName("SSAO.IntermediateColorTexture");

		RenderGraphPassSpecifications ssaoMainPass{};
		ssaoMainPass.SetDebugName("SSAOMainPass");
		ssaoMainPass.AddInput(viewport.NormalsTexture);
		ssaoMainPass.AddInput(viewport.DepthTexture);
		ssaoMainPass.AddOutput(aoTexture, 0);

		RenderGraphPassSpecifications ssaoComposingPass{};
		ssaoComposingPass.SetDebugName("SSAOComposingPass");
		ssaoComposingPass.AddInput(aoTexture);
		ssaoComposingPass.AddInput(viewport.ColorTexture);
		ssaoComposingPass.AddOutput(intermediateColorTexture, 0);

		RenderGraphPassSpecifications ssaoBlitPass{};
		ssaoBlitPass.SetDebugName("SSAOBlitPass");
		BlitPass::ConfigureSpecifications(ssaoBlitPass, intermediateColorTexture, viewport.ColorTexture);

		renderGraph.AddPass(ssaoMainPass, CreateRef<SSAOMainPass>(viewport.NormalsTexture, viewport.DepthTexture));
		renderGraph.AddPass(ssaoComposingPass, CreateRef<SSAOComposingPass>(viewport.ColorTexture, aoTexture));
		renderGraph.AddPass(ssaoBlitPass, CreateRef<BlitPass>(intermediateColorTexture, viewport.ColorTexture, TextureFiltering::Closest));
	}

	const SerializableObjectDescriptor& SSAO::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(SSAO);
	}



	SSAOMainPass::SSAOMainPass(Ref<Texture> normalsTexture, Ref<Texture> depthTexture)
		: m_NormalsTexture(normalsTexture), m_DepthTexture(depthTexture)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAO");
		if (shaderHandle && AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			m_Material = Material::Create(AssetManager::GetAsset<Shader>(shaderHandle.value()));
		}
		else
		{
			FLARE_CORE_ERROR("SSAO: Failed to find SSAO shader");
		}

		auto result = Scene::GetActive()->GetPostProcessingManager().GetEffect<SSAO>();
		FLARE_CORE_ASSERT(result.has_value());
		m_Parameters = *result;
	}

	void SSAOMainPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
			vulkanCommandBuffer->SetPrimaryDescriptorSet(Renderer::GetPrimaryDescriptorSet());
		}

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		auto biasIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.Bias");
		auto radiusIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.SampleRadius");

		auto normalsTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_NormalsTexture");
		auto depthTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");

		m_Material->WritePropertyValue(*biasIndex, m_Parameters->Bias);
		m_Material->WritePropertyValue(*radiusIndex, m_Parameters->Radius);
		m_Material->SetTextureProperty(*normalsTextureIndex, m_NormalsTexture);
		m_Material->SetTextureProperty(*depthTextureIndex, m_DepthTexture);

		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)context.GetViewport().GetSize()));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();
	}



	SSAOComposingPass::SSAOComposingPass(Ref<Texture> colorTexture, Ref<Texture> aoTexture)
		: m_ColorTexture(colorTexture), m_AOTexture(aoTexture)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAOBlur");
		if (shaderHandle && AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			m_Material = Material::Create(AssetManager::GetAsset<Shader>(shaderHandle.value()));
		}
		else
		{
			FLARE_CORE_ERROR("SSAO: Failed to find SSAO Blur shader");
		}

		auto result = Scene::GetActive()->GetPostProcessingManager().GetEffect<SSAO>();
		FLARE_CORE_ASSERT(result.has_value());
		m_Parameters = *result;
	}

	void SSAOComposingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)context.GetViewport().GetSize()));
		glm::vec2 texelSize = glm::vec2(1.0f) / (glm::vec2)context.GetViewport().GetSize();

		auto colorTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_ColorTexture");
		auto aoTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_AOTexture");
		auto blurSizePropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.BlurSize");
		auto texelSizePropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.TexelSize");

		m_Material->WritePropertyValue(*blurSizePropertyIndex, m_Parameters->BlurSize);
		m_Material->WritePropertyValue(*texelSizePropertyIndex, texelSize);
		m_Material->SetTextureProperty(*aoTextureIndex, m_AOTexture);
		m_Material->SetTextureProperty(*colorTextureIndex, m_ColorTexture);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();
	}
}
