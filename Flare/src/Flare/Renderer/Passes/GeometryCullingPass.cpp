#include "PCH.h"
#include "GeometryCullingPass.h"

#include "FlareECS/World.h"

#include "Flare/Math/SIMD.h"

#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/RenderGraph/RenderGraphContext.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RendererSubmitionQueue.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/RenderData.h"

namespace Flare
{
	FLARE_IMPL_COMPONENT(CulledGeometry);
	
	CulledGeometry::CulledGeometry()
	{
		FLARE_PROFILE_FUNCTION();
		constexpr size_t maxInstances = 16;

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			CulledGeometry::GPUFrameResources& resources = FrameResources.emplace_back();
			resources.InstanceBuffer = GPUBuffer::CreateStorageBuffer(maxInstances * sizeof(CulledGeometry::InstanceDataBuffer), GPUBufferMemoryType::Static);

			resources.InstanceBufferDescriptor = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
			resources.InstanceBufferDescriptor->WriteStorageBuffer(resources.InstanceBuffer, 0);
			resources.InstanceBufferDescriptor->FlushWrites();
		}
	}

	void CulledGeometry::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		VisibleObjects.clear();
		InstanceDataBuffer.clear();
		CulledBatches.clear();
	}

	CulledGeometry::GPUFrameResources::~GPUFrameResources()
	{
		if (InstanceBufferDescriptor)
			Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(InstanceBufferDescriptor);
	}

	void CulledGeometry::GPUFrameResources::Resize(size_t newTransformCount)
	{
		size_t instanceDataSize = sizeof(PackedTransform) * newTransformCount;
		if (instanceDataSize > InstanceBuffer->GetSize())
		{
			InstanceBuffer->Resize(instanceDataSize);
			InstanceBufferDescriptor->WriteStorageBuffer(InstanceBuffer, 0);
			InstanceBufferDescriptor->FlushWrites();
		}
	}

	void GeometryCullingPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const RendererSubmitionQueue& opaqueGeometry = context.GetSceneSubmition().OpaqueGeometrySubmitions;
		const GeometryBatcher& geometryBatcher = context.GetSceneSubmition().BatchedGeometry;

		const Viewport& viewport = context.RenderWorld.GetEntityComponent<const Viewport>(context.ViewportEntity);
		const ViewportGlobalResources& viewportResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		CulledGeometry& culledGeometry = context.RenderWorld.GetEntityComponent<CulledGeometry>(context.ViewportEntity);

		CulledGeometry::GPUFrameResources& frameResources = culledGeometry.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		culledGeometry.Clear();

		const RenderView& cameraView = context.GetRenderView();

		FrustumPlanes frustumPlanes;
		frustumPlanes.SetFromViewAndProjection(cameraView.View, cameraView.InverseViewProjection, cameraView.ViewDirection);

		size_t totalTransformCount = 0;
		for (const auto& [key, batch] : geometryBatcher.GetBatches())
		{
			for (size_t subMeshIndex = 0; subMeshIndex < batch.GetMesh()->GetSubMeshes().size(); subMeshIndex++)
			{
				CulledGeometryBatch culledBatch(subMeshIndex, batch);
				CullGeometryBatch(frustumPlanes, culledBatch);

				if (!culledBatch.CulledGeometryIndices.empty())
				{
					size_t transformCount = culledBatch.CulledGeometryIndices.size();
					culledBatch.TransformBufferOffset = totalTransformCount;
					culledGeometry.CulledBatches.push_back(std::move(culledBatch));
					totalTransformCount += transformCount;
				}
			}
		}

		std::vector<PackedTransform> transforms;
		transforms.reserve(totalTransformCount);

		for (const auto& batch : culledGeometry.CulledBatches)
		{
			for (uint32_t index : batch.CulledGeometryIndices)
			{
				transforms.push_back(batch.OriginalBatch->GetTransforms()[index]);
			}
		}

		frameResources.Resize(totalTransformCount);
		frameResources.InstanceBuffer->SetData(MemorySpan::FromVector(transforms), 0, commandBuffer);
	}

	void GeometryCullingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	inline static bool CheckFrustumVsAABBIntersection(const FrustumPlanes& planes, const Math::AABB& aabb)
	{
		for (size_t i = 0; i < planes.PlanesCount; i++)
		{
			if (!aabb.IntersectsOrInFrontOfPlane(planes.Planes[i]))
			{
				return false;
			}
		}

		return true;
	}

	void GeometryCullingPass::CullGeometry(const RenderGraphContext& context, std::vector<uint32_t>& culledGeometry)
	{
		FLARE_PROFILE_FUNCTION();

		const RenderView& cameraView = context.GetRenderView();

		FrustumPlanes planes{};
		planes.SetFromViewAndProjection(cameraView.View, cameraView.InverseViewProjection, cameraView.ViewDirection);

		const RendererSubmitionQueue& opaqueGeometry = context.GetSceneSubmition().OpaqueGeometrySubmitions;

		for (size_t i = 0; i < opaqueGeometry.GetSize(); i++)
		{
			const auto& object = opaqueGeometry[i];
			Math::AABB objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform.ToMatrix4x4());

			bool intersects = true;
			for (size_t i = 0; i < planes.PlanesCount; i++)
			{
				if (!objectAABB.IntersectsOrInFrontOfPlane(planes.Planes[i]))
				{
					intersects = false;
					break;
				}
			}

			if (intersects)
			{
				culledGeometry.push_back((uint32_t)i);
			}
		}
	}

	void GeometryCullingPass::CullGeometryBatch(const FrustumPlanes& frustumPlanes, CulledGeometryBatch& outCulledGeometry)
	{
		FLARE_PROFILE_FUNCTION();
		const auto& transforms = outCulledGeometry.OriginalBatch->GetTransforms();

		Math::AABB meshBounds;
		
		if (outCulledGeometry.SubMeshIndex == CulledGeometryBatch::ALL_SUBMESHES)
			meshBounds = outCulledGeometry.OriginalBatch->GetMesh()->GetBounds();
		else
			outCulledGeometry.OriginalBatch->GetMesh()->GetSubMeshes()[outCulledGeometry.SubMeshIndex].Bounds;

		for (size_t i = 0; i < transforms.size(); i++)
		{
			Math::AABB transformedAABB = meshBounds.Transformed(transforms[i].AsMatrix4x4());
			bool intersects = CheckFrustumVsAABBIntersection(frustumPlanes, transformedAABB);

			if (intersects)
			{
				outCulledGeometry.CulledGeometryIndices.push_back(static_cast<uint32_t>(i));
			}
		}
	}
}
