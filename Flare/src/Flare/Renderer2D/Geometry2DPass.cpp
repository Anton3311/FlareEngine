#include "Geometry2DPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Math/Math.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	Geometry2DPass::Geometry2DPass(const Renderer2DFrameData& frameData, const Renderer2DLimits& limits, Ref<IndexBuffer> indexBuffer, Ref<Material> defaultMaterial)
		: m_FrameData(frameData), m_RendererLimits(limits), m_IndexBuffer(indexBuffer), m_DefaultMaterial(defaultMaterial)
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
		FLARE_PROFILE_FUNCTION();

		if (m_FrameData.QuadCount > 0)
		{
			m_VertexBuffer->SetData(MemorySpan(const_cast<QuadVertex*>(m_FrameData.QuadVertices.data()), m_FrameData.QuadCount * 4), 0, commandBuffer);
		}

		ReleaseDescriptorSets();

		// NOTE: This pass should be removed from the RenderGraph
		//       if there are weren't anything submitted for rendering

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		vulkanCommandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>*)&m_VertexBuffer, 1));
		vulkanCommandBuffer->BindIndexBuffer(m_IndexBuffer);

		for (const auto& batch : m_FrameData.QuadBatches)
		{
			if (batch.Count == 0)
				continue;

			FlushBatch(batch, commandBuffer);
		}

		commandBuffer->EndRenderTarget();
	}

	void Geometry2DPass::ReleaseDescriptorSets()
	{
		FLARE_PROFILE_FUNCTION();
		for (auto set : m_UsedSets)
		{
			m_FrameData.QuadDescriptorSetsPool->ReleaseSet(set);
		}

		m_UsedSets.clear();
	}

	void Geometry2DPass::FlushBatch(const QuadsBatch& batch, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
			Ref<VulkanFrameBuffer> renderTarget = As<VulkanFrameBuffer>(Renderer::GetCurrentViewport().RenderTarget);

			Ref<DescriptorSet> descriptorSet = m_FrameData.QuadDescriptorSetsPool->AllocateSet();
			descriptorSet->SetDebugName("QuadsDescriptorSet");

			m_UsedSets.push_back(descriptorSet);

			descriptorSet->WriteImages(Span((Ref<const Texture>*)batch.Textures, Renderer2DLimits::MaxTexturesCount), 0, 0);
			descriptorSet->FlushWrites();

			vulkanCommandBuffer->SetPrimaryDescriptorSet(Renderer::GetPrimaryDescriptorSet());
			vulkanCommandBuffer->SetSecondaryDescriptorSet(descriptorSet);

			vulkanCommandBuffer->ApplyMaterial(batch.Material);
			vulkanCommandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f), (glm::vec2)renderTarget->GetSize()));
			vulkanCommandBuffer->DrawIndexed(batch.Start * 6, batch.Count * 6);
		}
	}
}
