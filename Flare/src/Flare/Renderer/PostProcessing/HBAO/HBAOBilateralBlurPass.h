#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/ShaderConstantBuffer.h"
#include "Flare/Renderer/ShaderDescriptorBuffer.h"

namespace Flare
{
	class ComputeShader;
	class SSAO;

	class HBAOBilateralBlurPass : public RenderGraphPass
	{
	public:
		HBAOBilateralBlurPass(Ref<SSAO> parameters,
			bool isVertical,
			RenderGraphTextureId linearDepthTexture,
			RenderGraphTextureId aoTexture,
			RenderGraphTextureId outputTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<SSAO> m_Parameters = nullptr;
		Ref<ComputeShader> m_Shader = nullptr;

		ShaderConstantBuffer m_ConstantBuffer;
		ShaderDescriptorBuffer m_DescriptorBuffer;

		RenderGraphTextureId m_LinearDepthTexture;
		RenderGraphTextureId m_AOTexture;
		RenderGraphTextureId m_OutputTexture;

		bool m_IsVertical = false;
	};
}
