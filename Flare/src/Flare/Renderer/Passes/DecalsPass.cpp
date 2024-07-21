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
		m_InstanceBuffer = ShaderStorageBuffer::Create(maxDecals * sizeof(InstanceData));

		m_DecalSet = m_DecalDescriptorPool->AllocateSet();
		m_ShouldUpdateDescriptorSet = true;

		m_InstanceDataDescriptor = Renderer::GetInstanceDataDescriptorSetPool()->AllocateSet();
		m_InstanceDataDescriptor->WriteStorageBuffer(m_InstanceBuffer, 0);
		m_InstanceDataDescriptor->FlushWrites();
	}

	DecalsPass::~DecalsPass()
	{
		m_DecalDescriptorPool->ReleaseSet(m_DecalSet);

		Renderer::GetInstanceDataDescriptorSetPool()->ReleaseSet(m_InstanceDataDescriptor);
	}

	void DecalsPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		//if (m_ShouldUpdateDescriptorSet)
		//{
			m_DecalSet->WriteImage(context.GetRenderGraph().GetTexture(m_DepthTexture), 0);
			m_DecalSet->FlushWrites();

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

		m_InstanceBuffer->SetData(MemorySpan::FromVector(m_InstanceData), 0, commandBuffer);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());
		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.CameraDescriptorSet, 0);
		commandBuffer->SetGlobalDescriptorSet(m_DecalSet, 1);
		commandBuffer->SetGlobalDescriptorSet(m_InstanceDataDescriptor, 2);

		Ref<const Mesh> cubeMesh = RendererPrimitives::GetCube();
		for (size_t decalIndex = 0; decalIndex < submittedDecals.size(); decalIndex++)
		{
			const DecalSubmition& decal = submittedDecals[decalIndex];

			commandBuffer->ApplyMaterial(decal.Material);
			commandBuffer->DrawMeshIndexed(cubeMesh, 0, (uint32_t)decalIndex, 1);
		}

		commandBuffer->EndRenderTarget();
	}
}
