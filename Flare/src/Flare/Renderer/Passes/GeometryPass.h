#pragma once

#include "Flare/Renderer/RendererStatistics.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include <vector>

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetPool;
	class GPUBuffer;
	class GPUTimer;
	class Material;
	class Mesh;

	class GeometryPass : public RenderGraphPass 
	{
	public:
		GeometryPass(RendererStatistics& statistics);

		~GeometryPass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		std::optional<float> GetElapsedTime() const;
	private:
		struct Batch
		{
			Ref<const Mesh> Mesh = nullptr;
			Ref<const Material> Material = nullptr;
			uint32_t SubMesh = 0;
			uint32_t BaseInstance = 0;
			uint32_t InstanceCount = 0;
		};

		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};
	
		void CullObjects(const RenderGraphContext& context);
		void FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch);
	private:
		struct FrameResources
		{
			Ref<GPUBuffer> InstanceBuffer = nullptr;
			Ref<DescriptorSet> InstanceBufferDescriptor = nullptr;
		};

		Ref<GPUTimer> m_Timer = nullptr;

		RendererStatistics& m_Statistics;
		std::vector<uint32_t> m_VisibleObjects;
		std::vector<InstanceData> m_InstanceData;

		std::vector<FrameResources> m_FrameResources;
	};
}
