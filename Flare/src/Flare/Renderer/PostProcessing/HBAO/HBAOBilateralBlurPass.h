#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Material;
	class SSAO;

	class HBAOBilateralBlurPass : public RenderGraphPass
	{
	public:
		HBAOBilateralBlurPass(Ref<SSAO> parameters, bool isVertical, RenderGraphTextureId linearDepthTexture, RenderGraphTextureId aoTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<SSAO> m_Parameters = nullptr;
		Ref<Material> m_Material = nullptr;

		RenderGraphTextureId m_LinearDepthTexture;
		RenderGraphTextureId m_AOTexture;

		bool m_IsVertical = false;
	};
}
