#pragma once

#include "Flare/Math/Transform.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RendererStatistics.h"

namespace Flare
{
	struct RenderView;
	struct ShadowCascadeData;
	struct VisibleSubMeshRange;

	class DescriptorSet;
	class DescriptorSetPool;
	class GPUTimer;
	class Mesh;
	class ShaderStorageBuffer;
	class UniformBuffer;

	class ShadowCascadePass : public RenderGraphPass
	{
	public:
		static constexpr size_t MaxCascades = 4;

		ShadowCascadePass(RendererStatistics& statistics,
			const ShadowCascadeData& cascadeData,
			const std::vector<Math::Compact3DTransform>& filteredTransforms,
			const std::vector<VisibleSubMeshRange>& visibleSubMeshRanges);

		~ShadowCascadePass();

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
		RendererStatistics& m_Statistics;

		const ShadowCascadeData& m_CascadeData;
		const std::vector<Math::Compact3DTransform>& m_FilteredTransforms;
		const std::vector<VisibleSubMeshRange>& m_VisibleSubMeshRanges;

		Ref<GPUTimer> m_Timer = nullptr;

		Ref<UniformBuffer> m_CameraBuffer = nullptr;
		Ref<DescriptorSet> m_CameraDescriptor = nullptr;

		Ref<ShaderStorageBuffer> m_InstanceBuffer = nullptr;
		Ref<DescriptorSet> m_InstanceBufferDescriptor = nullptr;

		std::vector<InstanceData> m_InstanceDataBuffer;
	};
}
