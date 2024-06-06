#include "Tonemapping.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/Renderer/Passes/BlitPass.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ToneMapping);

	ToneMapping::ToneMapping()
	{
	}

	void ToneMapping::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		TextureSpecifications colorTextureSpecifications{};
		colorTextureSpecifications.Filtering = TextureFiltering::Closest;
		colorTextureSpecifications.Format = TextureFormat::R11G11B10;
		colorTextureSpecifications.Wrap = TextureWrap::Clamp;
		colorTextureSpecifications.GenerateMipMaps = false;
		colorTextureSpecifications.Width = viewport.GetSize().x;
		colorTextureSpecifications.Height = viewport.GetSize().y;
		colorTextureSpecifications.Usage = TextureUsage::RenderTarget | TextureUsage::Sampling;

		Ref<Texture> intermediateTexture = Texture::Create(colorTextureSpecifications);

		RenderGraphPassSpecifications toneMappingPass{};
		toneMappingPass.SetDebugName("ToneMapping");
		toneMappingPass.AddInput(viewport.ColorTexture);
		toneMappingPass.AddOutput(intermediateTexture, 0);

		RenderGraphPassSpecifications blitPass{};
		blitPass.SetDebugName("ToneMappingBlit");
		blitPass.AddInput(intermediateTexture, ImageLayout::TransferSource);
		blitPass.AddOutput(viewport.ColorTexture, 0, ImageLayout::TransferDestination);

		renderGraph.AddPass(toneMappingPass, CreateRef<ToneMappingPass>(viewport.ColorTexture));
		renderGraph.AddPass(blitPass, CreateRef<BlitPass>(intermediateTexture, TextureFiltering::Closest));
	}

	const SerializableObjectDescriptor& ToneMapping::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(ToneMapping);
	}



	ToneMappingPass::ToneMappingPass(Ref<Texture> colorTexture)
		: m_ColorTexture(colorTexture)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("AcesToneMapping");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("ToneMapping: Failed to find ToneMapping shader");
			return;
		}

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
		m_Material = Material::Create(shader);

		
	}

	void ToneMappingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

#if 0
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_ColorTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
#endif

		auto colorTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_ScreenBuffer");

		if (colorTextureIndex)
			m_Material->GetPropertyValue<TexturePropertyValue>(*colorTextureIndex).SetTexture(m_ColorTexture);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());
		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)context.GetViewport().GetSize()));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();

#if 0
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_ColorTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif
	}
}
