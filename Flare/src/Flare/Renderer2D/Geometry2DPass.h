#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer2D/Renderer2DFrameData.h"

namespace Flare
{
	class VertexBuffer;
	class IndexBuffer;

	class Geometry2DPass : public RenderGraphPass
	{
	public:
		Geometry2DPass(const Renderer2DFrameData& frameData, const Renderer2DLimits& limits,
			Ref<IndexBuffer> indexBuffer, Ref<Material> defaultMaterial);
		~Geometry2DPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void ReleaseDescriptorSets();
		void FlushBatch(const RenderGraphContext& context, const QuadsBatch& batch, Ref<CommandBuffer> commandBuffer);
	private:
		const Renderer2DFrameData& m_FrameData;
		const Renderer2DLimits& m_RendererLimits;

		std::vector<Ref<DescriptorSet>> m_UsedSets;

		Ref<VertexBuffer> m_VertexBuffer = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;
		Ref<Material> m_DefaultMaterial = nullptr;
	};
}
