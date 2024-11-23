#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Bloom;
	class Material;

	class BloomLuminanceIsolationPass : public RenderGraphPass
	{
	public:
		BloomLuminanceIsolationPass(RenderGraphTextureId colorTexture, Ref<const Bloom> parameters);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_ColorTexture;
		Ref<Material> m_Material = nullptr;
		Ref<const Bloom> m_Parameters = nullptr;
	};
}
