#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class RenderGraphPassSpecifications;
	class FLARE_API BlitPass : public RenderGraphPass
	{
	public:
		BlitPass(RenderGraphTextureId sourceTexture, RenderGraphTextureId destination, TextureFiltering filter);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;

		static void ConfigureSpecifications(RenderGraphPassSpecifications& specifications, RenderGraphTextureId source, RenderGraphTextureId destination);
	private:
		TextureFiltering m_Filter = TextureFiltering::Closest;
		RenderGraphTextureId m_SourceTexture;
		RenderGraphTextureId m_Destination;
	};
}
