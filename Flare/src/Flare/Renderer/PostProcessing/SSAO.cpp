#include "SSAO.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include <random>

namespace Flare
{
	FLARE_IMPL_TYPE(SSAO);

	SSAO::SSAO()
		: RenderPass(RenderPassQueue::PostProcessing), m_BiasPropertyIndex({}), m_RadiusPropertyIndex({}), Bias(0.1f), Radius(0.5f), BlurSize(2.0f), Enabled(true)
	{
		{
			std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAO");
			if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
			{
				FLARE_CORE_ERROR("SSAO: Failed to find SSAO shader");
			}
			else
			{
				m_Material = Material::Create(AssetManager::GetAsset<Shader>(shaderHandle.value()));
				m_BiasPropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.Bias");
				m_RadiusPropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.SampleRadius");

				m_NormalsTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_NormalsTexture");
				m_DepthTextureIndex= m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");
			}
		}

		{
			std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAOBlur");
			if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
			{
				FLARE_CORE_ERROR("SSAO: Failed to find SSAO Blur shader");
				return;
			}
			else
			{
				m_BlurMaterial = Material::Create(AssetManager::GetAsset<Shader>(shaderHandle.value()));
				m_BlurSizePropertyIndex = m_BlurMaterial->GetShader()->GetPropertyIndex("u_Params.BlurSize");
				m_TexelSizePropertyIndex = m_BlurMaterial->GetShader()->GetPropertyIndex("u_Params.TexelSize");

				m_ColorTexture = m_BlurMaterial->GetShader()->GetPropertyIndex("u_ColorTexture");
				m_AOTexture = m_BlurMaterial->GetShader()->GetPropertyIndex("u_AOTexture");
			}
		}
	}

	void SSAO::OnRender(RenderingContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		if (!Enabled || !Renderer::GetCurrentViewport().PostProcessingEnabled)
			return;

		if (m_Material == nullptr || m_BlurMaterial == nullptr)
			return;

		TextureFormat formats[] = { TextureFormat::RF32 };
		TextureFormat colorFormats[] = { TextureFormat::RGB8 };
		Ref<FrameBuffer> intermediateAOTarget = context.RTPool.GetFullscreen(Span(formats, 1));
		Ref<FrameBuffer> intermediateColorTarget = context.RTPool.GetFullscreen(Span(colorFormats, 1));

		const Viewport& currentViewport = Renderer::GetCurrentViewport();
		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		glm::vec2 texelSize = glm::vec2(1.0f / currentViewport.GetSize().x, 1.0f / currentViewport.GetSize().y);

		{
			FLARE_PROFILE_SCOPE("SSAO::MainPass");

			if (m_NormalsTextureIndex)
				m_Material->GetPropertyValue<TexturePropertyValue>(*m_NormalsTextureIndex).SetFrameBuffer(currentViewport.RenderTarget, currentViewport.NormalsAttachmentIndex);

			if (m_DepthTextureIndex)
				m_Material->GetPropertyValue<TexturePropertyValue>(*m_DepthTextureIndex).SetFrameBuffer(currentViewport.RenderTarget, currentViewport.DepthAttachmentIndex);

			if (m_BiasPropertyIndex)
				m_Material->WritePropertyValue(*m_BiasPropertyIndex, Bias);
			if (m_RadiusPropertyIndex)
				m_Material->WritePropertyValue(*m_RadiusPropertyIndex, Radius);

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
				Ref<VulkanFrameBuffer> frameBuffer = As<VulkanFrameBuffer>(context.RenderTarget);
				vulkanCommandBuffer->TransitionDepthImageLayout(
					frameBuffer->GetAttachmentImage(currentViewport.DepthAttachmentIndex), true,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				vulkanCommandBuffer->TransitionImageLayout(
					frameBuffer->GetAttachmentImage(currentViewport.NormalsAttachmentIndex),
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			commandBuffer->BeginRenderTarget(intermediateAOTarget);

			commandBuffer->ApplyMaterial(m_Material);
			commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)currentViewport.GetSize().x, (float)currentViewport.GetSize().y));
			commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

			commandBuffer->EndRenderTarget();

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
				Ref<VulkanFrameBuffer> frameBuffer = As<VulkanFrameBuffer>(context.RenderTarget);
				vulkanCommandBuffer->TransitionDepthImageLayout(
					frameBuffer->GetAttachmentImage(currentViewport.DepthAttachmentIndex), true,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

				vulkanCommandBuffer->TransitionImageLayout(
					frameBuffer->GetAttachmentImage(currentViewport.NormalsAttachmentIndex),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

				vulkanCommandBuffer->TransitionImageLayout(
					As<VulkanFrameBuffer>(intermediateAOTarget)->GetAttachmentImage(0),
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				vulkanCommandBuffer->TransitionImageLayout(
					As<VulkanFrameBuffer>(context.RenderTarget)->GetAttachmentImage(currentViewport.ColorAttachmentIndex),
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		{
			FLARE_PROFILE_SCOPE("SSAO::BlurAndCombinePass");

			commandBuffer->BeginRenderTarget(intermediateColorTarget);

			if (m_AOTexture)
				m_BlurMaterial->GetPropertyValue<TexturePropertyValue>(*m_AOTexture).SetFrameBuffer(intermediateAOTarget, 0);
			if (m_ColorTexture)
				m_BlurMaterial->GetPropertyValue<TexturePropertyValue>(*m_ColorTexture).SetFrameBuffer(currentViewport.RenderTarget, currentViewport.ColorAttachmentIndex);

			if (m_BlurSizePropertyIndex)
				m_BlurMaterial->WritePropertyValue(*m_BlurSizePropertyIndex, BlurSize);
			if (m_TexelSizePropertyIndex)
				m_BlurMaterial->WritePropertyValue(*m_TexelSizePropertyIndex, texelSize);

			commandBuffer->ApplyMaterial(m_BlurMaterial);
			commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)currentViewport.GetSize().x, (float)currentViewport.GetSize().y));
			commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

			commandBuffer->EndRenderTarget();

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
				Ref<VulkanFrameBuffer> frameBuffer = As<VulkanFrameBuffer>(context.RenderTarget);

				vulkanCommandBuffer->TransitionImageLayout(
					As<VulkanFrameBuffer>(intermediateAOTarget)->GetAttachmentImage(0),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

				vulkanCommandBuffer->TransitionImageLayout(
					As<VulkanFrameBuffer>(context.RenderTarget)->GetAttachmentImage(currentViewport.ColorAttachmentIndex),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
		}

		commandBuffer->Blit(intermediateColorTarget, 0, context.RenderTarget, 0, TextureFiltering::Closest);

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
			Ref<VulkanFrameBuffer> frameBuffer = As<VulkanFrameBuffer>(context.RenderTarget);

			vulkanCommandBuffer->TransitionImageLayout(
				As<VulkanFrameBuffer>(intermediateColorTarget)->GetAttachmentImage(0),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		context.RTPool.ReturnFullscreen(Span(formats, 1), intermediateAOTarget);
		context.RTPool.ReturnFullscreen(Span(colorFormats, 1), intermediateColorTarget);
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
	}

	void SSAOMainPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_NormalsTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vulkanCommandBuffer->TransitionDepthImageLayout(
			As<VulkanTexture>(m_DepthTexture)->GetImageHandle(), true,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		auto biasIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.Bias");
		auto radiusIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.SampleRadius");

		auto normalsTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_NormalsTexture");
		auto depthTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");

		m_Material->WritePropertyValue(*biasIndex, 0.0001f);
		m_Material->WritePropertyValue(*radiusIndex, 0.5f);
		m_Material->GetPropertyValue<TexturePropertyValue>(*normalsTextureIndex).SetTexture(m_NormalsTexture);
		m_Material->GetPropertyValue<TexturePropertyValue>(*depthTextureIndex).SetTexture(m_DepthTexture);

		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)context.GetViewport().GetSize()));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();

		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_NormalsTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vulkanCommandBuffer->TransitionDepthImageLayout(
			As<VulkanTexture>(m_DepthTexture)->GetImageHandle(), true,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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
	}

	void SSAOComposingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_AOTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_ColorTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)context.GetViewport().GetSize()));
		glm::vec2 texelSize = glm::vec2(1.0f) / (glm::vec2)context.GetViewport().GetSize();

		auto colorTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_ColorTexture");
		auto aoTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_AOTexture");
		auto blurSizePropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.BlurSize");
		auto texelSizePropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.TexelSize");

		m_Material->WritePropertyValue(*blurSizePropertyIndex, 2.0f);
		m_Material->WritePropertyValue(*texelSizePropertyIndex, texelSize);
		m_Material->GetPropertyValue<TexturePropertyValue>(*aoTextureIndex).SetTexture(m_AOTexture);
		m_Material->GetPropertyValue<TexturePropertyValue>(*colorTextureIndex).SetTexture(m_ColorTexture);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();

		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_AOTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_ColorTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}



	SSAOBlitPass::SSAOBlitPass(Ref<Texture> colorTexture)
		: m_ColorTexture(colorTexture)
	{
	}

	void SSAOBlitPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		commandBuffer->Blit(m_ColorTexture, context.GetRenderTarget()->GetAttachment(0), TextureFiltering::Closest);

		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_ColorTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
}
