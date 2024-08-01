#include "ShadowCascadePass.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GPUTimer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererSubmitionQueue.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Renderer/Passes/ShadowPass.h"

#include "Flare/Math/Math.h"
#include "Flare/Math/SIMD.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	ShadowCascadePass::ShadowCascadePass(RendererStatistics& statistics,
		const ShadowCascadeData& cascadeData,
		const std::vector<Math::Compact3DTransform>& filteredTransforms,
		const std::vector<VisibleSubMeshRange>& visibleSubMeshRanges)
		: m_Statistics(statistics),
		m_CascadeData(cascadeData),
		m_FilteredTransforms(filteredTransforms),
		m_VisibleSubMeshRanges(visibleSubMeshRanges)
	{
		FLARE_PROFILE_FUNCTION();

		m_Timer = GPUTimer::Create();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		constexpr size_t maxInstanceCount = 16;
		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			FrameResources& resources = m_FrameResources.emplace_back();

			resources.CameraBuffer = UniformBuffer::Create(sizeof(RenderView));
			resources.CameraDescriptor = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
			resources.CameraDescriptor->WriteUniformBuffer(resources.CameraBuffer, 0);
			resources.CameraDescriptor->FlushWrites();

			resources.InstanceBuffer = ShaderStorageBuffer::Create(maxInstanceCount * sizeof(InstanceData));
			resources.InstanceBufferDescriptor = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
			resources.InstanceBufferDescriptor->WriteStorageBuffer(resources.InstanceBuffer, 0);
			resources.InstanceBufferDescriptor->FlushWrites();
		}
	}

	ShadowCascadePass::~ShadowCascadePass()
	{
		for (const FrameResources& resources : m_FrameResources)
		{
			Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(resources.InstanceBufferDescriptor);
			Renderer::GetCameraDescriptorSetPool()->ReleaseSet(resources.CameraDescriptor);
		}
	}

	void ShadowCascadePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& currentViewport = context.GetViewport();
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		const FrameResources& resources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		if (m_CascadeData.Batches.size() == 0 && m_CascadeData.PartiallyVisible.size() == 0)
		{
			commandBuffer->BeginRenderTarget(context.GetRenderTarget());
			commandBuffer->EndRenderTarget();
			return;
		}

		resources.CameraBuffer->SetData(&m_CascadeData.View, sizeof(m_CascadeData.View), 0);

		m_Statistics.ShadowPassTime += m_Timer->GetElapsedTime().value_or(0.0f);

		{
			FLARE_PROFILE_SCOPE("FillInstanceData");

			m_InstanceDataBuffer.clear();

			const glm::mat4& viewProjection = m_CascadeData.View.ViewProjection;
			for (const auto& batch : m_CascadeData.Batches)
			{
				for (uint32_t i = 0; i < batch.Count; i++)
				{
					auto& instanceData = m_InstanceDataBuffer.emplace_back();
					instanceData.TransformAndViewProjection = viewProjection * m_FilteredTransforms[batch.FirstEntryIndex + i].ToMatrix4x4();
				}
			}

			for (const auto& visibleMesh : m_CascadeData.PartiallyVisible)
			{
				auto& instanceData = m_InstanceDataBuffer.emplace_back();
				instanceData.TransformAndViewProjection = viewProjection * visibleMesh.Transform.ToMatrix4x4();
			}
		}

		size_t instanceDataSize = sizeof(InstanceData) * m_InstanceDataBuffer.size();
		if (instanceDataSize > resources.InstanceBuffer->GetSize())
		{
			resources.InstanceBuffer->Resize(instanceDataSize);
			resources.InstanceBufferDescriptor->WriteStorageBuffer(resources.InstanceBuffer, 0);
			resources.InstanceBufferDescriptor->FlushWrites();
		}

		resources.InstanceBuffer->SetData(MemorySpan::FromVector(m_InstanceDataBuffer), 0, commandBuffer);

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

		const FrameResources& resources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		commandBuffer->SetGlobalDescriptorSet(resources.CameraDescriptor, 0);
		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().GlobalDescriptorSetWithoutShadows, 1);
		commandBuffer->SetGlobalDescriptorSet(resources.InstanceBufferDescriptor, 2);

		commandBuffer->ApplyMaterial(Renderer::GetDepthOnlyMaterial());

		uint32_t instanceIndex = 0;

		{
			FLARE_PROFILE_SCOPE("DrawFullyVisible");

			for (const auto& batch : m_CascadeData.Batches)
			{
				commandBuffer->DrawMeshIndexed(batch.Mesh, instanceIndex, batch.Count);
				instanceIndex += batch.Count;

				m_Statistics.DrawCallCount++;
			}
		}

		{
			FLARE_PROFILE_SCOPE("DrawPartiallyVisible");

			Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
			for (const auto& visibleMesh : m_CascadeData.PartiallyVisible)
			{
				for (uint32_t i = 0; i < visibleMesh.SubMeshRangeCount; i++)
				{
					VisibleSubMeshRange range = m_VisibleSubMeshRanges[visibleMesh.FirstSubMeshRange + i];
					vulkanCommandBuffer->DrawMeshIndexed(visibleMesh.Mesh, range.Start, range.Count, instanceIndex, 1);

					m_Statistics.DrawCallCount++;
				}

				instanceIndex++;
			}
		}
	}
}
