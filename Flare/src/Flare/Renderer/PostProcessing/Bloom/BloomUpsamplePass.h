#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Bloom;
	class Material;
	class Sampler;
	class BloomUpsamplePass : public RenderGraphPass
	{
	public:
		BloomUpsamplePass(Ref<const Bloom> parameters, RenderGraphTextureId sourceTexture, RenderGraphTextureId previousMip);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_SourceTexture;
		RenderGraphTextureId m_PreviousMip;
		Ref<const Bloom> m_Parameters;
		Ref<Material> m_Material = nullptr;
		Ref<Sampler> m_Sampler = nullptr;
	};
}
