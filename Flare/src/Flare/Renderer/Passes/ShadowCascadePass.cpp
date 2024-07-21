#include "ShadowCascadePass.h"

#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GPUTimer.h"

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

		m_CameraBuffer = UniformBuffer::Create(sizeof(RenderView));
		m_CameraDescriptor = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
		m_CameraDescriptor->WriteUniformBuffer(m_CameraBuffer, 0);
		m_CameraDescriptor->FlushWrites();

		constexpr size_t maxInstanceCount = 16;
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

		if (m_CascadeData.Batches.size() == 0 && m_CascadeData.PartiallyVisible.size() == 0)
		{
			commandBuffer->BeginRenderTarget(context.GetRenderTarget());
			commandBuffer->EndRenderTarget();
			return;
		}

		m_CameraBuffer->SetData(&m_CascadeData.View, sizeof(m_CascadeData.View), 0);

		m_Statistics.ShadowPassTime += m_Timer->GetElapsedTime().value_or(0.0f);

		{
			FLARE_PROFILE_SCOPE("FillInstanceData");

			m_InstanceDataBuffer.clear();

			for (const auto& batch : m_CascadeData.Batches)
			{
				for (uint32_t i = 0; i < batch.Count; i++)
				{
					auto& instanceData = m_InstanceDataBuffer.emplace_back();
					const auto& transform = m_FilteredTransforms[batch.FirstEntryIndex + i];
					instanceData.PackedTransform[0] = glm::vec4(transform.RotationScale[0], transform.Translation.x);
					instanceData.PackedTransform[1] = glm::vec4(transform.RotationScale[1], transform.Translation.y);
					instanceData.PackedTransform[2] = glm::vec4(transform.RotationScale[2], transform.Translation.z);
				}
			}

			for (const auto& visibleMesh : m_CascadeData.PartiallyVisible)
			{
				auto& instanceData = m_InstanceDataBuffer.emplace_back();
				const auto& transform = visibleMesh.Transform;
				instanceData.PackedTransform[0] = glm::vec4(transform.RotationScale[0], transform.Translation.x);
				instanceData.PackedTransform[1] = glm::vec4(transform.RotationScale[1], transform.Translation.y);
				instanceData.PackedTransform[2] = glm::vec4(transform.RotationScale[2], transform.Translation.z);
			}
		}

		size_t instanceDataSize = sizeof(InstanceData) * m_InstanceDataBuffer.size();
		if (instanceDataSize > m_InstanceBuffer->GetSize())
		{
			m_InstanceBuffer->Resize(instanceDataSize);
			m_InstanceBufferDescriptor->WriteStorageBuffer(m_InstanceBuffer, 0);
			m_InstanceBufferDescriptor->FlushWrites();
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
