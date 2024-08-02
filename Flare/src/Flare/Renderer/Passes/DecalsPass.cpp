#include "DecalsPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	DecalsPass::DecalsPass(Ref<DescriptorSetPool> decalDescriptorPool, RenderGraphTextureId depthTexture)
		: m_DepthTexture(depthTexture), m_DecalDescriptorPool(decalDescriptorPool)
	{
		const size_t maxDecals = 1000;
		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			FrameResources& frameResources = m_FrameResources.emplace_back();

			frameResources.InstanceBuffer = ShaderStorageBuffer::Create(maxDecals * sizeof(InstanceData));

			frameResources.DecalSet = m_DecalDescriptorPool->AllocateSet();
			m_ShouldUpdateDescriptorSet = true;

			frameResources.InstanceBufferDescriptor = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
			frameResources.InstanceBufferDescriptor->WriteStorageBuffer(frameResources.InstanceBuffer, 0);
			frameResources.InstanceBufferDescriptor->FlushWrites();
		}
	}

	DecalsPass::~DecalsPass()
	{
		for (FrameResources& resources : m_FrameResources)
		{
			m_DecalDescriptorPool->ReleaseSet(resources.DecalSet);
			Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(resources.InstanceBufferDescriptor);
		}
	}

	void DecalsPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		//if (m_ShouldUpdateDescriptorSet)
		//{
			frameResources.DecalSet->WriteImage(context.GetRenderGraph().GetTexture(m_DepthTexture), 0);
			frameResources.DecalSet->FlushWrites();

			m_ShouldUpdateDescriptorSet = false;
		//}

		const auto& submittedDecals = context.GetSceneSubmition().DecalSubmitions;

		{
			FLARE_PROFILE_SCOPE("FillInstanceData");

			m_InstanceData.clear();
			m_InstanceData.reserve(submittedDecals.size());

			for (const DecalSubmition& decal : submittedDecals)
			{
				InstanceData& instanceData = m_InstanceData.emplace_back();
				instanceData.PackedTransform[0] = glm::vec4(decal.Transform.RotationScale[0], decal.Transform.Translation.x);
				instanceData.PackedTransform[1] = glm::vec4(decal.Transform.RotationScale[1], decal.Transform.Translation.y);
				instanceData.PackedTransform[2] = glm::vec4(decal.Transform.RotationScale[2], decal.Transform.Translation.z);
			}
		}

		frameResources.InstanceBuffer->SetData(MemorySpan::FromVector(m_InstanceData), 0, commandBuffer);

	}

	void DecalsPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const auto& submittedDecals = context.GetSceneSubmition().DecalSubmitions;

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);
		commandBuffer->SetGlobalDescriptorSet(frameResources.DecalSet, 1);
		commandBuffer->SetGlobalDescriptorSet(frameResources.InstanceBufferDescriptor, 2);

		Ref<const Mesh> cubeMesh = RendererPrimitives::GetCube();
		for (size_t decalIndex = 0; decalIndex < submittedDecals.size(); decalIndex++)
		{
			const DecalSubmition& decal = submittedDecals[decalIndex];

			commandBuffer->ApplyMaterial(decal.Material);
			commandBuffer->DrawMeshIndexed(cubeMesh, 0, (uint32_t)decalIndex, 1);
		}
	}
}
