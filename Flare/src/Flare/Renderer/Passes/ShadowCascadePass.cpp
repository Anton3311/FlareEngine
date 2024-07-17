#include "ShadowCascadePass.h"

#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GPUTimer.h"

#include "Flare/Math/Math.h"
#include "Flare/Math/SIMD.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	ShadowCascadePass::ShadowCascadePass(const RendererSubmitionQueue& opaqueObjects,
		RendererStatistics& statistics,
		const RenderView& lightView,
		const std::vector<uint32_t>& visibleObjects,
		Ref<Texture> cascadeTexture)
		: m_OpaqueObjects(opaqueObjects),
		m_Statistics(statistics),
		m_LightView(lightView),
		m_VisibleObjects(visibleObjects),
		m_CascadeTexture(cascadeTexture)
	{
		FLARE_PROFILE_FUNCTION();

		m_Timer = GPUTimer::Create();

		m_CameraBuffer = UniformBuffer::Create(sizeof(RenderView));
		m_CameraDescriptor = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
		m_CameraDescriptor->WriteUniformBuffer(m_CameraBuffer, 0);
		m_CameraDescriptor->FlushWrites();

		constexpr size_t maxInstanceCount = 1000;
		m_InstanceBuffer = ShaderStorageBuffer::Create(maxInstanceCount * sizeof(InstanceData));
		m_InstanceBufferDescriptor = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
		m_InstanceBufferDescriptor->WriteStorageBuffer(m_InstanceBuffer, 0);
		m_InstanceBufferDescriptor->FlushWrites();
	}

	ShadowCascadePass::~ShadowCascadePass()
	{
		Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(m_InstanceBufferDescriptor);
		Renderer::GetCameraDescriptorSetPool()->ReleaseSet(m_CameraDescriptor);
	}

	void ShadowCascadePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& currentViewport = context.GetViewport();
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		if (m_VisibleObjects.size() == 0)
		{
			commandBuffer->BeginRenderTarget(context.GetRenderTarget());
			commandBuffer->EndRenderTarget();
			return;
		}

		m_CameraBuffer->SetData(&m_LightView, sizeof(m_LightView), 0);

		m_Statistics.ShadowPassTime += m_Timer->GetElapsedTime().value_or(0.0f);

#if 0
		{
			FLARE_PROFILE_SCOPE("GroupByMesh");

			// Group objects with same meshes together
			std::sort(perCascadeObjects[cascadeIndex].begin(),
				perCascadeObjects[cascadeIndex].end(),
				[this](uint32_t aIndex, uint32_t bIndex) -> bool
				{
					const auto& a = m_OpaqueObjects[aIndex];
					const auto& b = m_OpaqueObjects[bIndex];

					if (a.Mesh.get() == a.Mesh.get())
						return a.SubMeshIndex < b.SubMeshIndex;

					return (uint64_t)a.Mesh.get() < (uint64_t)b.Mesh.get();
				});
		}
#endif

		{
			FLARE_PROFILE_SCOPE("FillInstanceData");

			m_InstanceDataBuffer.clear();
			for (uint32_t i : m_VisibleObjects)
			{
				auto& instanceData = m_InstanceDataBuffer.emplace_back();
				const auto& transform = m_OpaqueObjects[i].Transform;
				instanceData.PackedTransform[0] = glm::vec4(transform.RotationScale[0], transform.Translation.x);
				instanceData.PackedTransform[1] = glm::vec4(transform.RotationScale[1], transform.Translation.y);
				instanceData.PackedTransform[2] = glm::vec4(transform.RotationScale[2], transform.Translation.z);
			}
		}

		m_InstanceBuffer->SetData(MemorySpan::FromVector(m_InstanceDataBuffer), 0, commandBuffer);

		commandBuffer->StartTimer(m_Timer);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());
		uint32_t shadowMapResolution = GetShadowMapResolution(shadowSettings.Quality);
		commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)shadowMapResolution, (float)shadowMapResolution));

		DrawCascade(context, commandBuffer);

		commandBuffer->EndRenderTarget();

		commandBuffer->StopTimer(m_Timer);
	}

	void ShadowCascadePass::DrawCascade(const RenderGraphContext& context, const Ref<CommandBuffer>& commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->SetGlobalDescriptorSet(m_CameraDescriptor, 0);
		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.GlobalDescriptorSetWithoutShadows, 1);
		commandBuffer->SetGlobalDescriptorSet(m_InstanceBufferDescriptor, 2);

		commandBuffer->ApplyMaterial(Renderer::GetDepthOnlyMaterial());

		Batch batch{};
		for (uint32_t objectIndex : m_VisibleObjects)
		{
			const auto& object = m_OpaqueObjects[objectIndex];
			if (object.Mesh.get() != batch.Mesh.get() || object.SubMeshIndex != batch.SubMesh)
			{
				FlushBatch(commandBuffer, batch);

				batch.BaseInstance += batch.InstanceCount;
				batch.InstanceCount = 0;
				batch.Mesh = object.Mesh;
				batch.SubMesh = object.SubMeshIndex;
			}

			batch.InstanceCount++;
		}

		// Draw remaining objects
		batch.InstanceCount = (uint32_t)m_VisibleObjects.size() - batch.BaseInstance;
		FlushBatch(commandBuffer, batch);
	}

	void ShadowCascadePass::FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch)
	{
		FLARE_PROFILE_FUNCTION();

		if (batch.InstanceCount == 0)
			return;

		m_Statistics.DrawCallCount++;
		m_Statistics.DrawCallsSavedByInstancing += batch.InstanceCount - 1;

		commandBuffer->DrawMeshIndexed(batch.Mesh, batch.SubMesh, batch.BaseInstance, batch.InstanceCount);
	}
}
