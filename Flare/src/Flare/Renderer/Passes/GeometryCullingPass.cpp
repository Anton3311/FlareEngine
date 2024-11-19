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

	CulledGeometry::GPUFrameResources::~GPUFrameResources()
	{
		if (InstanceBufferDescriptor)
			Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(InstanceBufferDescriptor);
	}

	void GeometryCullingPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		const RendererSubmitionQueue& opaqueGeometry = context.GetSceneSubmition().OpaqueGeometrySubmitions;

		const Viewport& viewport = context.RenderWorld.GetEntityComponent<const Viewport>(context.ViewportEntity);
		const ViewportGlobalResources& viewportResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		CulledGeometry& culledGeometry = context.RenderWorld.GetEntityComponent<CulledGeometry>(context.ViewportEntity);

		const CulledGeometry::GPUFrameResources& frameResources = culledGeometry.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		culledGeometry.VisibleObjects.clear();

		CullGeometry(context, culledGeometry.VisibleObjects);

		{
			FLARE_PROFILE_SCOPE("Sort");
			std::sort(culledGeometry.VisibleObjects.begin(), culledGeometry.VisibleObjects.end(), [&opaqueGeometry](uint32_t a, uint32_t b) -> bool
			{
				return opaqueGeometry[a].SortKey < opaqueGeometry[b].SortKey;
			});
		}

		culledGeometry.InstanceDataBuffer.clear();

		{
			FLARE_PROFILE_SCOPE("FillInstanceData");
			for (uint32_t objectIndex : culledGeometry.VisibleObjects)
			{
				auto& instanceData = culledGeometry.InstanceDataBuffer.emplace_back();
				const auto& transform = opaqueGeometry[objectIndex].Transform;
				instanceData.PackedTransform[0] = glm::vec4(transform.RotationScale[0], transform.Translation.x);
				instanceData.PackedTransform[1] = glm::vec4(transform.RotationScale[1], transform.Translation.y);
				instanceData.PackedTransform[2] = glm::vec4(transform.RotationScale[2], transform.Translation.z);
			}
		}

		size_t instanceDataSize = sizeof(CulledGeometry::InstanceData) * culledGeometry.InstanceDataBuffer.size();
		if (instanceDataSize > frameResources.InstanceBuffer->GetSize())
		{
			frameResources.InstanceBuffer->Resize(instanceDataSize);
			frameResources.InstanceBufferDescriptor->WriteStorageBuffer(frameResources.InstanceBuffer, 0);
			frameResources.InstanceBufferDescriptor->FlushWrites();
		}

		frameResources.InstanceBuffer->SetData(MemorySpan::FromVector(culledGeometry.InstanceDataBuffer), 0, commandBuffer);
	}

	void GeometryCullingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void GeometryCullingPass::CullGeometry(const RenderGraphContext& context, std::vector<uint32_t>& culledGeometry)
	{
		FLARE_PROFILE_FUNCTION();

		Math::AABB objectAABB;

		const RenderView& cameraView = context.GetRenderView();

		FrustumPlanes planes{};
		planes.SetFromViewAndProjection(cameraView.View, cameraView.InverseViewProjection, cameraView.ViewDirection);

		const RendererSubmitionQueue& opaqueGeometry = context.GetSceneSubmition().OpaqueGeometrySubmitions;

		for (size_t i = 0; i < opaqueGeometry.GetSize(); i++)
		{
			const auto& object = opaqueGeometry[i];
			objectAABB = Math::SIMD::TransformAABB(object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds, object.Transform.ToMatrix4x4());

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
}
