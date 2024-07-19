#include "BlitPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/FrameBuffer.h"

namespace Flare
{
	BlitPass::BlitPass(RenderGraphTextureId sourceTexture, RenderGraphTextureId destination, TextureFiltering filter)
		: m_SourceTexture(sourceTexture), m_Destination(destination), m_Filter(filter)
	{
	}

	void BlitPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		commandBuffer->Blit(
			context.GetRenderGraphResourceManager().GetTexture(m_SourceTexture),
			context.GetRenderGraphResourceManager().GetTexture(m_Destination),
			m_Filter);
	}

	void BlitPass::ConfigureSpecifications(RenderGraphPassSpecifications& specifications, RenderGraphTextureId source, RenderGraphTextureId destination)
	{
		FLARE_PROFILE_FUNCTION();
		specifications.SetType(RenderGraphPassType::Other);
		specifications.AddInput(source, ImageLayout::TransferSource);
		specifications.AddOutput(destination, 0, ImageLayout::TransferDestination);
	}
}
