#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	struct TextBatch;
	struct Renderer2DLimits;

	class DescriptorSet;
	class DescriptorSetPool;
	class IndexBuffer;
	class Pipeline;
	class Shader;
	class VertexBuffer;

	class TextPass : public RenderGraphPass
	{
	public:
		TextPass(const Renderer2DLimits& limits, Ref<IndexBuffer> indexBuffer, Ref<Shader> textShader, Ref<DescriptorSetPool> descriptorSetPool);
		~TextPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void FlushBatch(const TextBatch& batch, Ref<CommandBuffer> commandBuffer);
		void ReleaseDescriptorSets();
	private:
		const Renderer2DLimits& m_RendererLimits;
		Ref<Shader> m_TextShader = nullptr;
		Ref<Pipeline> m_TextPipeline = nullptr;

		Ref<VertexBuffer> m_VertexBuffer = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;

		Ref<DescriptorSetPool> m_DescriptorSetPool = nullptr;

		std::vector<Ref<DescriptorSet>> m_UsedSets;
	};
}
