#pragma once

#include "Flare/Renderer/GeometryBatcher.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "FlareECS/Entity/ComponentInitializer.h"

namespace Flare
{
	class DescriptorSet;
	struct FrustumPlanes;
	class GeometryBatch;
	class GPUBuffer;
	class World;

	struct CulledGeometryBatch
	{
		FLARE_NONCOPYABLE(CulledGeometryBatch);

		static constexpr size_t ALL_SUBMESHES = SIZE_MAX;

		CulledGeometryBatch(size_t subMeshIndex, const GeometryBatch& originalBatch)
			: SubMeshIndex(subMeshIndex), OriginalBatch(&originalBatch) {}

		CulledGeometryBatch(CulledGeometryBatch&&) = default;
		CulledGeometryBatch& operator=(CulledGeometryBatch&&) = default;
	public:
		size_t SubMeshIndex = ALL_SUBMESHES;
		size_t TransformBufferOffset = 0;

		const GeometryBatch* OriginalBatch;
		std::vector<uint32_t> CulledGeometryIndices;
	};

	struct FLARE_API CulledGeometry
	{
		FLARE_COMPONENT;
		FLARE_NONCOPYABLE(CulledGeometry);

		CulledGeometry();

		CulledGeometry(CulledGeometry&&) = default;
		CulledGeometry& operator=(CulledGeometry&&) = default;

		void Clear();

		struct FLARE_API GPUFrameResources
		{
			FLARE_NONCOPYABLE(GPUFrameResources);

			GPUFrameResources() = default;

			GPUFrameResources(GPUFrameResources&&) = default;
			GPUFrameResources& operator=(GPUFrameResources&&) = default;

			~GPUFrameResources();

			void Resize(size_t newTransformCount);

			Ref<GPUBuffer> InstanceBuffer = nullptr;
			Ref<DescriptorSet> InstanceBufferDescriptor = nullptr;
		};

		std::vector<uint32_t> VisibleObjects;
		std::vector<PackedTransform> InstanceDataBuffer;
		std::vector<GPUFrameResources> FrameResources;

		std::vector<CulledGeometryBatch> CulledBatches;
	};

	class GeometryCullingPass : public RenderGraphPass
	{
	public:
		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		static void CullGeometry(const RenderGraphContext& context, std::vector<uint32_t>& culledGeometry);
		static void CullGeometryBatch(const FrustumPlanes& frustumPlanes, CulledGeometryBatch& outCulledGeometry);
	};
}
