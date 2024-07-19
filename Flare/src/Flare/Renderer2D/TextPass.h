#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/Renderer2D/Renderer2DFrameData.h"

namespace Flare
{
	class Shader;
	class Pipeline;
	class VertexBuffer;
	class IndexBuffer;

	class TextPass : public RenderGraphPass
	{
	public:
		TextPass(const Renderer2DFrameData& frameData, const Renderer2DLimits& limits, Ref<IndexBuffer> indexBuffer, Ref<Shader> textShader);
		~TextPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void FlushBatch(const TextBatch& batch, Ref<CommandBuffer> commandBuffer);
		void ReleaseDescriptorSets();
	private:
		const Renderer2DFrameData& m_FrameData;
		const Renderer2DLimits& m_RendererLimits;
		Ref<Shader> m_TextShader = nullptr;
		Ref<Pipeline> m_TextPipeline = nullptr;

		Ref<VertexBuffer> m_VertexBuffer = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;

		std::vector<Ref<DescriptorSet>> m_UsedSets;
	};
}
