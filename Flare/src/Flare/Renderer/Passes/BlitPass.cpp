#include "BlitPass.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/FrameBuffer.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	BlitPass::BlitPass(Ref<Texture> sourceTexture, Ref<Texture> destination, TextureFiltering filter)
		: m_SourceTexture(sourceTexture), m_Destination(destination), m_Filter(filter)
	{
	}

	void BlitPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

#if 0
		// NOTE: Blit transitions the source texture from COLOR_ATTACHMENT_OUTPUT_OPTIMAL to TRANSFER_SRC
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_SourceTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif

		commandBuffer->Blit(m_SourceTexture, m_Destination, m_Filter);

#if 0
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_SourceTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif
	}

	void BlitPass::ConfigureSpecifications(RenderGraphPassSpecifications& specifications, Ref<Texture> source, Ref<Texture> destination)
	{
		specifications.SetType(RenderGraphPassType::Other);
		specifications.AddInput(source, ImageLayout::TransferSource);
		specifications.AddOutput(destination, 0, ImageLayout::TransferDestination);
	}
}
