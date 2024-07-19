#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class FLARE_API BlitPass : public RenderGraphPass
	{
	public:
		BlitPass(Ref<Texture> sourceTexture, Ref<Texture> destination, TextureFiltering filter);
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;

		static void ConfigureSpecifications(RenderGraphPassSpecifications& specifications, Ref<Texture> source, Ref<Texture> destination);
	private:
		TextureFiltering m_Filter = TextureFiltering::Closest;
		Ref<Texture> m_SourceTexture = nullptr;
		Ref<Texture> m_Destination = nullptr;
	};
}
