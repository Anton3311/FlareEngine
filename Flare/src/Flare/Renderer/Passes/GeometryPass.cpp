#include "GeometryPass.h"

#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/GPUTimer.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Math/SIMD.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	GeometryPass::GeometryPass(const RendererSubmitionQueue& opaqueObjects, RendererStatistics& statistics)
		: m_OpaqueObjects(opaqueObjects), m_Statistics(statistics)
	{
		FLARE_PROFILE_FUNCTION();
		constexpr size_t maxInstances = 1000;
		m_InstanceStorageBuffer = ShaderStorageBuffer::Create(maxInstances * sizeof(InstanceData));

		m_InstanceDataDescriptor = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
		m_InstanceDataDescriptor->WriteStorageBuffer(m_InstanceStorageBuffer, 0);
		m_InstanceDataDescriptor->FlushWrites();

		m_Timer = GPUTimer::Create();
	}

	GeometryPass::~GeometryPass()
	{
		Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(m_InstanceDataDescriptor);
	}

	void GeometryPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.CameraDescriptorSet, 0);

		if (context.GetViewport().IsShadowMappingEnabled() && Renderer::GetShadowSettings().Enabled)
			commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.GlobalDescriptorSet, 1);
		else
			commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.GlobalDescriptorSetWithoutShadows, 1);

		commandBuffer->SetGlobalDescriptorSet(m_InstanceDataDescriptor, 2);

		m_VisibleObjects.clear();

		CullObjects();

		m_Statistics.GeometryPassTime += m_Timer->GetElapsedTime().value_or(0.0f); // Read the time from the previous frame
		m_Statistics.ObjectsVisible += (uint32_t)m_VisibleObjects.size();

		{
			FLARE_PROFILE_SCOPE("Sort");
			std::sort(m_VisibleObjects.begin(), m_VisibleObjects.end(), [this](uint32_t a, uint32_t b) -> bool
			{
				return m_OpaqueObjects[a].SortKey < m_OpaqueObjects[b].SortKey;
			});
		}

		m_InstanceBuffer.clear();

		{
			FLARE_PROFILE_SCOPE("FillInstacesData");
			for (uint32_t objectIndex : m_VisibleObjects)
			{
				auto& instanceData = m_InstanceBuffer.emplace_back();
				const auto& transform = m_OpaqueObjects[objectIndex].Transform;
				instanceData.PackedTransform[0] = glm::vec4(transform.RotationScale[0], transform.Translation.x);
				instanceData.PackedTransform[1] = glm::vec4(transform.RotationScale[1], transform.Translation.y);
				instanceData.PackedTransform[2] = glm::vec4(transform.RotationScale[2], transform.Translation.z);
			}
		}

		m_InstanceStorageBuffer->SetData(MemorySpan::FromVector(m_InstanceBuffer), 0, commandBuffer);

		Ref<FrameBuffer> renderTarget = context.GetRenderTarget();

		commandBuffer->StartTimer(m_Timer);
		commandBuffer->BeginRenderTarget(renderTarget);
		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)renderTarget->GetSize()));

		Batch batch{};

		for (uint32_t currentInstance = 0; currentInstance < (uint32_t)m_VisibleObjects.size(); currentInstance++)
		{
			uint32_t objectIndex = m_VisibleObjects[currentInstance];
			const auto& object = m_OpaqueObjects[objectIndex];

			if (batch.Mesh.get() != object.Mesh.get()
				|| batch.SubMesh != object.SubMeshIndex)
			{
				batch.InstanceCount = currentInstance - batch.BaseInstance;

				FlushBatch(commandBuffer, batch);

				batch.BaseInstance = currentInstance;
				batch.InstanceCount = 0;
				batch.Mesh = object.Mesh;
				batch.SubMesh = object.SubMeshIndex;
			}

			if (object.Material.get() != batch.Material.get())
			{
				batch.InstanceCount = currentInstance - batch.BaseInstance;

				FlushBatch(commandBuffer, batch);
				batch.BaseInstance = currentInstance;
				batch.InstanceCount = 0;
				batch.Material = object.Material;
			}
		}

		batch.InstanceCount = (uint32_t)m_VisibleObjects.size() - batch.BaseInstance;
		FlushBatch(commandBuffer, batch);

		commandBuffer->EndRenderTarget();
		commandBuffer->StopTimer(m_Timer);
	}

	std::optional<float> GeometryPass::GetElapsedTime() const
	{
		return m_Timer->GetElapsedTime();
	}

	void GeometryPass::CullObjects()
	{
		FLARE_PROFILE_FUNCTION();

		Math::AABB objectAABB;

		const FrustumPlanes& planes = Renderer::GetCurrentViewport().FrameData.CameraFrustumPlanes;
		for (size_t i = 0; i < m_OpaqueObjects.GetSize(); i++)
		{
			const auto& object = m_OpaqueObjects[i];
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
				m_VisibleObjects.push_back((uint32_t)i);
			}
		}
	}

	void GeometryPass::FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch)
	{
		FLARE_PROFILE_FUNCTION();

		if (batch.InstanceCount == 0)
			return;

		m_Statistics.DrawCallCount++;
		m_Statistics.DrawCallsSavedByInstancing += batch.InstanceCount - 1;

		if (batch.Material == nullptr)
			commandBuffer->ApplyMaterial(Renderer::GetErrorMaterial());
		else
			commandBuffer->ApplyMaterial(batch.Material);

		commandBuffer->DrawMeshIndexed(batch.Mesh, batch.SubMesh, batch.BaseInstance, batch.InstanceCount);
	}
}
