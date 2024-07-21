#include "Geometry2DPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Math/Math.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "Flare/Renderer2D/Renderer2DFrameData.h"

#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	Geometry2DPass::Geometry2DPass(const Renderer2DLimits& limits,
		Ref<IndexBuffer> indexBuffer,
		Ref<Material> defaultMaterial,
		Ref<DescriptorSetPool> quadsDescriptorSetPool)
		: m_RendererLimits(limits), m_IndexBuffer(indexBuffer), m_DefaultMaterial(defaultMaterial), m_QuadsDescriptorSetPool(quadsDescriptorSetPool)
	{
		m_VertexBuffer = VertexBuffer::Create(sizeof(QuadVertex) * 4 * m_RendererLimits.MaxQuadCount, GPUBufferUsage::Static);
		As<VulkanVertexBuffer>(m_VertexBuffer)->GetBuffer().EnsureAllocated(); // HACk: To avoid binding NULL buffer
	}

	Geometry2DPass::~Geometry2DPass()
	{
		ReleaseDescriptorSets();
	}

	void Geometry2DPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		// NOTE: This pass should be removed from the RenderGraph
		//       if there are weren't anything submitted for rendering

		FLARE_PROFILE_FUNCTION();

		const Renderer2DFrameData& submition = context.GetSceneSubmition().Renderer2DSubmition;
		if (submition.QuadCount > 0)
		{
			m_VertexBuffer->SetData(MemorySpan(const_cast<QuadVertex*>(submition.QuadVertices.data()), submition.QuadCount * 4), 0, commandBuffer);
		}

		ReleaseDescriptorSets();

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GlobalResources.CameraDescriptorSet, 0);
		commandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>*)&m_VertexBuffer, 1), 0);
		commandBuffer->BindIndexBuffer(m_IndexBuffer);

		for (const auto& batch : submition.QuadBatches)
		{
			if (batch.Count == 0)
				continue;

			FlushBatch(context, batch, commandBuffer);
		}

		commandBuffer->EndRenderTarget();
	}

	void Geometry2DPass::ReleaseDescriptorSets()
	{
		FLARE_PROFILE_FUNCTION();

		for (auto set : m_UsedSets)
		{
			m_QuadsDescriptorSetPool->ReleaseSet(set);
		}

		m_UsedSets.clear();
	}

	void Geometry2DPass::FlushBatch(const RenderGraphContext& context, const QuadsBatch& batch, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<FrameBuffer> renderTarget = context.GetRenderTarget();

		Ref<DescriptorSet> descriptorSet = m_QuadsDescriptorSetPool->AllocateSet();
		descriptorSet->SetDebugName("QuadsDescriptorSet");
		descriptorSet->WriteImages(Span((Ref<const Texture>*)batch.Textures, Renderer2DLimits::MaxTexturesCount), 0, 0);
		descriptorSet->FlushWrites();

		m_UsedSets.push_back(descriptorSet);

		commandBuffer->SetGlobalDescriptorSet(descriptorSet, 1);
		commandBuffer->ApplyMaterial(batch.Material);

		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f), (glm::vec2)renderTarget->GetSize()));
		commandBuffer->DrawIndexed(batch.Start * 6, batch.Count * 6, 0, 0, 1);
	}
}
