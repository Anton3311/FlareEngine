#include "TextPass.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Font.h"

#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"

namespace Flare
{
	TextPass::TextPass(const Renderer2DFrameData& frameData, const Renderer2DLimits& limits, Ref<IndexBuffer> indexBuffer, Ref<Shader> textShader)
		: m_FrameData(frameData), m_RendererLimits(limits), m_IndexBuffer(indexBuffer), m_TextShader(textShader)
	{
		m_VertexBuffer = VertexBuffer::Create(sizeof(TextVertex) * 4 * m_RendererLimits.MaxQuadCount, GPUBufferUsage::Static);
		As<VulkanVertexBuffer>(m_VertexBuffer)->GetBuffer().EnsureAllocated(); // HACk: To avoid binding NULL buffer
	}

	TextPass::~TextPass()
	{
		ReleaseDescriptorSets();
	}

	void TextPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		ReleaseDescriptorSets();

		if (m_FrameData.TextQuadCount > 0)
		{
			m_VertexBuffer->SetData(MemorySpan(const_cast<TextVertex*>(m_FrameData.TextVertices.data()), m_FrameData.TextQuadCount * 4), 0, commandBuffer);
		}

		Ref<FrameBuffer> renderTarget = context.GetRenderTarget();

		if (m_TextPipeline == nullptr)
		{
			PipelineSpecifications specificaionts{};
			specificaionts.Shader = m_TextShader;
			specificaionts.Culling = CullingMode::Back;
			specificaionts.DepthTest = false;
			specificaionts.DepthWrite = false;
			specificaionts.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 0, 1, ShaderDataType::Float4 }, // COlor
				{ 0, 2, ShaderDataType::Float2 }, // UV
				{ 0, 3, ShaderDataType::Int }, // Entity index
			});

			Ref<const DescriptorSetLayout> layouts[] =
			{
				Renderer::GetPrimaryDescriptorSetLayout(),
				m_FrameData.TextDescriptorSetsPool->GetLayout()
			};

			m_TextPipeline = CreateRef<VulkanPipeline>(specificaionts,
				As<VulkanFrameBuffer>(renderTarget)->GetCompatibleRenderPass(),
				Span<Ref<const DescriptorSetLayout>>(layouts, 2),
				Span<ShaderPushConstantsRange>());
		}

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());
		
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		vulkanCommandBuffer->BindPipeline(m_TextPipeline);
		vulkanCommandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f), (glm::vec2)renderTarget->GetSize()));
		
		for (const auto& batch : m_FrameData.TextBatches)
		{
			if (batch.Count == 0)
				continue;

			FlushBatch(batch, commandBuffer);
		}

		commandBuffer->EndRenderTarget();
	}

	void TextPass::FlushBatch(const TextBatch& batch, Ref<CommandBuffer> commandBuffer)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		Ref<DescriptorSet> set = m_FrameData.TextDescriptorSetsPool->AllocateSet();
		m_UsedSets.push_back(set);

		set->SetDebugName("TextDescriptorSet");
		set->WriteImage(batch.Font->GetAtlas(), 0);
		set->FlushWrites();

		VkPipelineLayout pipelineLayout = As<const VulkanPipeline>(m_TextPipeline)->GetLayoutHandle();

		vulkanCommandBuffer->BindDescriptorSet(As<VulkanDescriptorSet>(Renderer::GetPrimaryDescriptorSet()), pipelineLayout, 0);
		vulkanCommandBuffer->BindDescriptorSet(As<VulkanDescriptorSet>(set), pipelineLayout, 1);

		vulkanCommandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>)m_VertexBuffer));
		vulkanCommandBuffer->BindIndexBuffer(m_IndexBuffer);
		vulkanCommandBuffer->DrawIndexed(batch.Start * 6, batch.Count * 6);
	}

	void TextPass::ReleaseDescriptorSets()
	{
		for (const auto& set : m_UsedSets)
		{
			m_FrameData.TextDescriptorSetsPool->ReleaseSet(set);
		}

		m_UsedSets.clear();
	}
}
