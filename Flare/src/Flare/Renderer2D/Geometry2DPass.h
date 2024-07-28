#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	struct Renderer2DLimits;
	struct QuadsBatch;

	class DescriptorSet;
	class DescriptorSetPool;
	class IndexBuffer;
	class VertexBuffer;
	class Material;

	class Geometry2DPass : public RenderGraphPass
	{
	public:
		Geometry2DPass(const Renderer2DLimits& limits,
			Ref<IndexBuffer> indexBuffer,
			Ref<Material> defaultMaterial,
			Ref<DescriptorSetPool> quadsDescriptorSetPool);
		~Geometry2DPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void ReleaseDescriptorSets(std::vector<Ref<DescriptorSet>>& sets);
		void FlushBatch(const RenderGraphContext& context, const QuadsBatch& batch, Ref<CommandBuffer> commandBuffer);
	private:
		struct FrameResources
		{
			std::vector<Ref<DescriptorSet>> UsedSets;

			Ref<VertexBuffer> VertexBuffer = nullptr;
		};

		const Renderer2DLimits& m_RendererLimits;

		std::vector<FrameResources> m_FrameResources;

		Ref<DescriptorSetPool> m_QuadsDescriptorSetPool = nullptr;
		Ref<Material> m_DefaultMaterial = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;
	};
}
