#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Material;
	class Sampler;
	class BloomDownsamplePass : public RenderGraphPass
	{
	public:
		BloomDownsamplePass(RenderGraphTextureId sourceTexture,
			Ref<Sampler> sampler,
			bool reduceDynamicRange);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_SourceTexture;
		Ref<Material> m_Material = nullptr;
		Ref<Sampler> m_Sampler = nullptr;
		bool m_ReduceDynamicRange;
	};
}
