#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Material;
	class SSAO;

	class HBAOPass : public RenderGraphPass
	{
	public:
		HBAOPass(Ref<SSAO> parameters, RenderGraphTextureId normalTexture, RenderGraphTextureId downsampledDepth);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<Material> m_Material = nullptr;
		RenderGraphTextureId m_DownsampledDepth;
		RenderGraphTextureId m_NormalTexture;

		Ref<SSAO> m_Parameters;
	};
}
