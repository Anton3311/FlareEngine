#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/ShaderConstantBuffer.h"
#include "Flare/Renderer/ShaderDescriptorBuffer.h"

namespace Flare
{
	class ComputeShader;

	class HBAOLinearizeDepthPass : public RenderGraphPass
	{
	public:
		HBAOLinearizeDepthPass(RenderGraphTextureId depthTexture, RenderGraphTextureId linearDepthTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_DepthTexture;
		RenderGraphTextureId m_LinearDepthTexture;

		Ref<ComputeShader> m_ComputeShader = nullptr;

		ShaderConstantBuffer m_ConstantBuffer;
		ShaderDescriptorBuffer m_DescriptorBuffer;
	};
}
