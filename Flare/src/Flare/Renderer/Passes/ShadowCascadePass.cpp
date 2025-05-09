#include "PCH.h"

#include "ShadowCascadePass.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GPUTimer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"

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

			resources.CameraBuffer = GPUBuffer::CreateUniformBuffer(sizeof(RenderView));
			resources.CameraDescriptor = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
			resources.CameraDescriptor->WriteUniformBuffer(resources.CameraBuffer, 0);
			resources.CameraDescriptor->FlushWrites();

			resources.InstanceBuffer = GPUBuffer::CreateStorageBuffer(maxInstanceCount * sizeof(InstanceData), GPUBufferMemoryType::Static);
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

	void ShadowCascadePass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& viewport = context.RenderWorld.GetEntityComponent<const Viewport>(context.ViewportEntity);
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		const FrameResources& resources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		if (m_CascadeData.Batches.size() == 0 && m_CascadeData.PartiallyVisible.size() == 0)
			return;

		resources.CameraBuffer->SetData(MemorySpan(&m_CascadeData.View, 1), 0);

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
					glm::mat4 transformAndViewProjection = viewProjection * m_FilteredTransforms[batch.FirstEntryIndex + i].ToMatrix4x4();
					glm::vec3 translation = transformAndViewProjection[3];

					instanceData.PackedTransform[0] = glm::vec4((glm::vec3)transformAndViewProjection[0], translation.x);
					instanceData.PackedTransform[1] = glm::vec4((glm::vec3)transformAndViewProjection[1], translation.y);
					instanceData.PackedTransform[2] = glm::vec4((glm::vec3)transformAndViewProjection[2], translation.z);
				}
			}

			for (const auto& visibleMesh : m_CascadeData.PartiallyVisible)
			{
				auto& instanceData = m_InstanceDataBuffer.emplace_back();
				glm::mat4 transformAndViewProjection = viewProjection * visibleMesh.Transform.ToMatrix4x4();

				glm::vec3 translation = transformAndViewProjection[3];

				instanceData.PackedTransform[0] = glm::vec4((glm::vec3)transformAndViewProjection[0], translation.x);
				instanceData.PackedTransform[1] = glm::vec4((glm::vec3)transformAndViewProjection[1], translation.y);
				instanceData.PackedTransform[2] = glm::vec4((glm::vec3)transformAndViewProjection[2], translation.z);
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
	}

	void ShadowCascadePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		//commandBuffer->StartTimer(m_Timer);

		uint32_t shadowMapResolution = GetShadowMapResolution(shadowSettings.Quality);
		commandBuffer->SetViewportAndScissors(Math::Rect(0.0f, 0.0f, (float)shadowMapResolution, (float)shadowMapResolution));

		DrawCascade(context, commandBuffer);

		//commandBuffer->StopTimer(m_Timer);
	}

	void ShadowCascadePass::DrawCascade(const RenderGraphContext& context, const Ref<CommandBuffer>& commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const FrameResources& resources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const ViewportGlobalResources& globalResources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);

		commandBuffer->SetGlobalDescriptorSet(resources.CameraDescriptor, 0);
		commandBuffer->SetGlobalDescriptorSet(globalResources.GetCurrentFrameResources().GlobalDescriptorSetWithoutShadows, 1);
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

			VulkanCommandBuffer& vulkanCommandBuffer = commandBuffer.DerefAs<VulkanCommandBuffer>();
			for (const auto& visibleMesh : m_CascadeData.PartiallyVisible)
			{
				for (uint32_t i = 0; i < visibleMesh.SubMeshRangeCount; i++)
				{
					VisibleSubMeshRange range = m_VisibleSubMeshRanges[visibleMesh.FirstSubMeshRange + i];
					Ref<const Mesh> mesh = visibleMesh.Mesh;

					vulkanCommandBuffer.BindMesh(visibleMesh.Mesh, MeshType::DepthOnly);

					uint32_t firstSubMesh = static_cast<uint32_t>(range.Start);
					uint32_t subMeshCount = static_cast<uint32_t>(range.Count);

					const auto& subMeshes = visibleMesh.Mesh->GetDepthOnlySubMeshes();

					if (mesh->GetSharedMesh() == nullptr)
					{
						uint32_t lastSubMeshIndex = firstSubMesh + subMeshCount;
						uint32_t indexCount = 0;

						if (lastSubMeshIndex == (uint32_t)subMeshes.size())
						{
							indexCount = (uint32_t)mesh->GetIndexCount() - subMeshes[firstSubMesh].BaseIndex;
						}
						else
						{
							indexCount = subMeshes[lastSubMeshIndex].BaseIndex - subMeshes[firstSubMesh].BaseIndex;
						}

						vkCmdDrawIndexed(vulkanCommandBuffer.GetHandle(), indexCount, 1, subMeshes[firstSubMesh].BaseIndex, 0, instanceIndex);
					}
					else
					{
						for (uint32_t i = firstSubMesh; i < subMeshCount; i++)
						{
							const SubMesh& subMesh = subMeshes[i];
							vkCmdDrawIndexed(vulkanCommandBuffer.GetHandle(), subMesh.IndicesCount, 1, subMesh.BaseIndex, subMesh.BaseVertex, instanceIndex);
						}
					}

					m_Statistics.DrawCallCount++;
				}

				instanceIndex++;
			}
		}
	}
}
