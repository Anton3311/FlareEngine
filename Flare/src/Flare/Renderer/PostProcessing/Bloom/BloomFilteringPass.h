#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Material;
	class Sampler;
	class BloomFilteringPass : public RenderGraphPass
	{
	public:
		BloomFilteringPass(RenderGraphTextureId sourceTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_SourceTexture;
		Ref<Material> m_Material = nullptr;
		Ref<Sampler> m_Sampler = nullptr;
	};
}
