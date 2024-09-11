#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Material;

	class HBAODownsamplePass : public RenderGraphPass
	{
	public:
		HBAODownsamplePass(RenderGraphTextureId depthTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_DepthTexture;
		Ref<Material> m_Material = nullptr;
	};
}
