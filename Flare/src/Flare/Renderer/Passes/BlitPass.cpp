#include "BlitPass.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/FrameBuffer.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	BlitPass::BlitPass(Ref<Texture> sourceTexture, TextureFiltering filter)
		: m_SourceTexture(sourceTexture), m_Filter(filter)
	{
	}

	void BlitPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		commandBuffer->Blit(m_SourceTexture, context.GetRenderTarget()->GetAttachment(0), m_Filter);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		vulkanCommandBuffer->TransitionImageLayout(
			As<VulkanTexture>(m_SourceTexture)->GetImageHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
}
