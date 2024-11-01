#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Bloom;
	class Material;
	class Sampler;
	class BloomBlitPass : public RenderGraphPass
	{
	public:
		BloomBlitPass(RenderGraphTextureId sourceTexture, Ref<const Bloom> parameters);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_SourceTexture;
		Ref<const Bloom> m_Parameters = nullptr;
		Ref<Material> m_Material = nullptr;
		Ref<Sampler> m_Sampler = nullptr;
	};
}
