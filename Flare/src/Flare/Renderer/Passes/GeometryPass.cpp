#include "PCH.h"

#include "GeometryPass.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/GPUTimer.h"
#include "Flare/Renderer/Passes/GeometryCullingPass.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#include "Flare/Math/SIMD.h"

namespace Flare
{
	GeometryPass::GeometryPass(RendererStatistics& statistics, Ref<Material> materialOverride, bool isDepthOnly)
		: m_Statistics(statistics), m_MaterialOverride(materialOverride), m_IsDepthOnly(isDepthOnly)
	{
	}

	void GeometryPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
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

		commandBuffer->SetDefaultViewportAndScissors();

		if (m_MaterialOverride)
			commandBuffer->ApplyMaterial(m_MaterialOverride);

		VulkanCommandBuffer& vulkanCommandBuffer = commandBuffer.DerefAs<VulkanCommandBuffer>();
		Ref<Material> errorMaterial = Renderer::GetErrorMaterial();

		for (const auto& culledBatch : culledGeometry.CulledBatches)
		{
			const Ref<const Mesh>& mesh = culledBatch.OriginalBatch->GetMesh();

			if (!m_MaterialOverride)
			{
				if (culledBatch.SubMeshIndex < culledBatch.OriginalBatch->GetMaterials().size())
					commandBuffer->ApplyMaterial(culledBatch.OriginalBatch->GetMaterials()[culledBatch.SubMeshIndex]);
				else
					commandBuffer->ApplyMaterial(errorMaterial);
			}

			vulkanCommandBuffer.BindMesh(mesh);
			if (m_IsDepthOnly)
			{
				vulkanCommandBuffer.BindIndexBuffer(mesh->GetDepthOnlyIndexBuffer(), mesh->GetIndexFormat());
			}

			SubMesh subMesh;
			if (m_IsDepthOnly)
			{
				subMesh = mesh->GetDepthOnlySubMeshes()[culledBatch.SubMeshIndex];
			}
			else
			{
				subMesh = mesh->GetSubMeshes()[culledBatch.SubMeshIndex];
			}

			vkCmdDrawIndexed(vulkanCommandBuffer.GetHandle(),
				subMesh.IndicesCount,
				static_cast<uint32_t>(culledBatch.CulledGeometryIndices.size()),
				subMesh.BaseIndex,
				subMesh.BaseVertex,
				static_cast<uint32_t>(culledBatch.TransformBufferOffset));
		}
	}
}
