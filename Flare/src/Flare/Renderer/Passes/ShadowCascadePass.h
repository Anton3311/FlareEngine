#pragma once

#include "Flare/Math/AffineTransform.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RendererStatistics.h"

namespace Flare
{
	struct RenderView;
	struct ShadowCascadeData;
	struct VisibleSubMeshRange;

	class DescriptorSet;
	class DescriptorSetPool;
	class GPUBuffer;
	class GPUTimer;
	class Mesh;

	class ShadowCascadePass : public RenderGraphPass
	{
	public:
		static constexpr size_t MaxCascades = 4;

		ShadowCascadePass(RendererStatistics& statistics,
			const ShadowCascadeData& cascadeData,
			const std::vector<Math::Compact3DTransform>& filteredTransforms,
			const std::vector<VisibleSubMeshRange>& visibleSubMeshRanges);

		~ShadowCascadePass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	public:
		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};

		struct Batch
		{
			Ref<const Mesh> Mesh = nullptr;
			uint32_t SubMesh = 0;
			uint32_t BaseInstance = 0;
			uint32_t InstanceCount = 0;
		};
	private:
		void DrawCascade(const RenderGraphContext& context, const Ref<CommandBuffer>& commandBuffer);
	private:
		struct FrameResources
		{
			Ref<GPUBuffer> CameraBuffer = nullptr;
			Ref<DescriptorSet> CameraDescriptor = nullptr;

			Ref<GPUBuffer> InstanceBuffer = nullptr;
			Ref<DescriptorSet> InstanceBufferDescriptor = nullptr;
		};

		RendererStatistics& m_Statistics;

		const ShadowCascadeData& m_CascadeData;
		const std::vector<Math::Compact3DTransform>& m_FilteredTransforms;
		const std::vector<VisibleSubMeshRange>& m_VisibleSubMeshRanges;

		Ref<GPUTimer> m_Timer = nullptr;

		std::vector<FrameResources> m_FrameResources;
		std::vector<InstanceData> m_InstanceDataBuffer;
	};
}
