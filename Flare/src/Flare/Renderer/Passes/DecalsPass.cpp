#include "DecalsPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	DecalsPass::DecalsPass(const std::vector<DecalSubmitionData>& submitedDecals, Ref<DescriptorSetPool> decalDescriptorPool, RenderGraphTextureId depthTexture)
		: m_SubmitedDecals(submitedDecals), m_DepthTexture(depthTexture), m_DecalDescriptorPool(decalDescriptorPool)
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

		if (m_ShouldUpdateDescriptorSet)
		{
			m_DecalSet->WriteImage(context.GetRenderGraph().GetTexture(m_DepthTexture), 0);
			m_DecalSet->FlushWrites();

			m_ShouldUpdateDescriptorSet = false;
		}

		{
			FLARE_PROFILE_SCOPE("FillInstanceData");
			m_InstanceData.clear();
			m_InstanceData.reserve(m_SubmitedDecals.size());

			for (const DecalSubmitionData& decal : m_SubmitedDecals)
			{
				InstanceData& instanceData = m_InstanceData.emplace_back();
				instanceData.PackedTransform[0] = decal.PackedTransform[0];
				instanceData.PackedTransform[1] = decal.PackedTransform[1];
				instanceData.PackedTransform[2] = decal.PackedTransform[2];
			}
		}

		m_InstanceBuffer->SetData(MemorySpan::FromVector(m_InstanceData), 0, commandBuffer);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());
		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.CameraDescriptorSet, 0);
		commandBuffer->SetGlobalDescriptorSet(m_DecalSet, 1);
		commandBuffer->SetGlobalDescriptorSet(m_InstanceDataDescriptor, 2);

		Ref<const Mesh> cubeMesh = RendererPrimitives::GetCube();
		for (size_t decalIndex = 0; decalIndex < m_SubmitedDecals.size(); decalIndex++)
		{
			const DecalSubmitionData& decal = m_SubmitedDecals[decalIndex];

			commandBuffer->ApplyMaterial(decal.Material);
			commandBuffer->DrawMeshIndexed(cubeMesh, 0, (uint32_t)decalIndex, 1);
		}

		commandBuffer->EndRenderTarget();
	}
}
