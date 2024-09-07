#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	struct TextBatch;
	struct Renderer2DLimits;

	class DescriptorSet;
	class DescriptorSetPool;
	class GPUBuffer;
	class Pipeline;
	class Shader;

	class TextPass : public RenderGraphPass
	{
	public:
		TextPass(const Renderer2DLimits& limits, Ref<GPUBuffer> indexBuffer, Ref<Shader> textShader, Ref<DescriptorSetPool> descriptorSetPool);
		~TextPass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void FlushBatch(const TextBatch& batch, Ref<CommandBuffer> commandBuffer);
		void ReleaseDescriptorSets(std::vector<Ref<DescriptorSet>>& sets);
	private:
		struct FrameResources
		{
			Ref<GPUBuffer> VertexBuffer = nullptr;
			std::vector<Ref<DescriptorSet>> UsedSets;
		};

		const Renderer2DLimits& m_RendererLimits;

		Ref<Shader> m_TextShader = nullptr;
		Ref<Pipeline> m_TextPipeline = nullptr;
		Ref<GPUBuffer> m_IndexBuffer = nullptr;
		Ref<DescriptorSetPool> m_DescriptorSetPool = nullptr;

		std::vector<FrameResources> m_FrameResources;
	};
}
