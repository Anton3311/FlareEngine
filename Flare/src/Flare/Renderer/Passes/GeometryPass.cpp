#include "PCH.h"

#include "GeometryPass.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/GPUTimer.h"
#include "Flare/Renderer/Passes/GeometryCullingPass.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Math/SIMD.h"

namespace Flare
{
	GeometryPass::GeometryPass(RendererStatistics& statistics, Ref<Material> materialOverride)
		: m_Statistics(statistics), m_MaterialOverride(materialOverride)
	{
		FLARE_PROFILE_FUNCTION();
	}

	void GeometryPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
	}

	void GeometryPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const CulledGeometry& culledGeometry = context.RenderWorld.GetEntityComponent<const CulledGeometry>(context.ViewportEntity);
		const CulledGeometry::GPUFrameResources& frameResources = culledGeometry.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		const Viewport* viewport = context.RenderWorld.TryGetEntityComponent<const Viewport>(context.ViewportEntity);
		const ViewportGlobalResources* viewportResources = context.RenderWorld.TryGetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		FLARE_CORE_ASSERT(viewportResources && viewport);

		commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().CameraDescriptorSet, 0);

		if (viewport->Settings.ShadowMappingEnabled && Renderer::GetShadowSettings().Enabled)
			commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().GlobalDescriptorSet, 1);
		else
			commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().GlobalDescriptorSetWithoutShadows, 1);

		commandBuffer->SetGlobalDescriptorSet(frameResources.InstanceBufferDescriptor, 2);

		const RendererSubmitionQueue& opaqueGeometry = context.GetSceneSubmition().OpaqueGeometrySubmitions;

		commandBuffer->SetDefaultViewportAndScissors();

		Batch batch{};
		batch.Material = m_MaterialOverride;

		for (uint32_t currentInstance = 0; currentInstance < (uint32_t)culledGeometry.VisibleObjects.size(); currentInstance++)
		{
			uint32_t objectIndex = culledGeometry.VisibleObjects[currentInstance];
			const auto& object = opaqueGeometry[objectIndex];

			if (batch.Mesh != object.Mesh
				|| batch.SubMesh != object.SubMeshIndex)
			{
				batch.InstanceCount = currentInstance - batch.BaseInstance;

				FlushBatch(commandBuffer, batch);

				batch.BaseInstance = currentInstance;
				batch.InstanceCount = 0;
				batch.Mesh = object.Mesh;
				batch.SubMesh = object.SubMeshIndex;
			}

			if (!m_MaterialOverride && object.Material != batch.Material)
			{
				batch.InstanceCount = currentInstance - batch.BaseInstance;

				FlushBatch(commandBuffer, batch);
				batch.BaseInstance = currentInstance;
				batch.InstanceCount = 0;
				batch.Material = object.Material;
			}
		}

		batch.InstanceCount = (uint32_t)culledGeometry.VisibleObjects.size() - batch.BaseInstance;
		FlushBatch(commandBuffer, batch);
	}

	void GeometryPass::FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch)
	{
		FLARE_PROFILE_FUNCTION();

		if (batch.InstanceCount == 0)
			return;

		m_Statistics.DrawCallCount++;
		m_Statistics.DrawCallsSavedByInstancing += batch.InstanceCount - 1;

		if (batch.Material == nullptr || batch.Material->GetShader() == nullptr)
			commandBuffer->ApplyMaterial(Renderer::GetErrorMaterial());
		else
			commandBuffer->ApplyMaterial(batch.Material);

		commandBuffer->DrawMeshIndexed(batch.Mesh, batch.SubMesh, batch.BaseInstance, batch.InstanceCount);
	}
}
