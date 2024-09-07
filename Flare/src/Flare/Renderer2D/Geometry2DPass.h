#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	struct Renderer2DLimits;
	struct QuadsBatch;

	class DescriptorSet;
	class DescriptorSetPool;
	class GPUBuffer;
	class Material;

	class Geometry2DPass : public RenderGraphPass
	{
	public:
		Geometry2DPass(const Renderer2DLimits& limits,
			Ref<GPUBuffer> indexBuffer,
			Ref<Material> defaultMaterial,
			Ref<DescriptorSetPool> quadsDescriptorSetPool);
		~Geometry2DPass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void ReleaseDescriptorSets(std::vector<Ref<DescriptorSet>>& sets);
		void FlushBatch(const RenderGraphContext& context, const QuadsBatch& batch, Ref<CommandBuffer> commandBuffer);
	private:
		struct FrameResources
		{
			std::vector<Ref<DescriptorSet>> UsedSets;

			Ref<GPUBuffer> VertexBuffer = nullptr;
		};

		const Renderer2DLimits& m_RendererLimits;

		std::vector<FrameResources> m_FrameResources;

		Ref<DescriptorSetPool> m_QuadsDescriptorSetPool = nullptr;
		Ref<Material> m_DefaultMaterial = nullptr;
		Ref<GPUBuffer> m_IndexBuffer = nullptr;
	};
}
