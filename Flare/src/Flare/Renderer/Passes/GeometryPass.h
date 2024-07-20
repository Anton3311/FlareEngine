#pragma once

#include "Flare/Renderer/RendererSubmitionQueue.h"
#include "Flare/Renderer/RendererStatistics.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include <vector>

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetPool;
	class ShaderStorageBuffer;
	class GPUTimer;

	class GeometryPass : public RenderGraphPass 
	{
	public:
		GeometryPass(const RendererSubmitionQueue& opaqueObjects, RendererStatistics& statistics);

		~GeometryPass();

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
		Ref<GPUTimer> m_Timer = nullptr;

		RendererStatistics& m_Statistics;
		const RendererSubmitionQueue& m_OpaqueObjects;
		std::vector<uint32_t> m_VisibleObjects;
		std::vector<InstanceData> m_InstanceBuffer;

		Ref<ShaderStorageBuffer> m_InstanceStorageBuffer = nullptr;
		Ref<DescriptorSet> m_InstanceDataDescriptor = nullptr;
	};
}
