#include "TextPass.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Renderer2D/Renderer2DFrameData.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"

namespace Flare
{
	TextPass::TextPass(const Renderer2DLimits& limits, Ref<IndexBuffer> indexBuffer, Ref<Shader> textShader, Ref<DescriptorSetPool> descriptorSetPool)
		: m_RendererLimits(limits), m_IndexBuffer(indexBuffer), m_TextShader(textShader), m_DescriptorSetPool(descriptorSetPool)
	{
		FLARE_PROFILE_FUNCTION();
		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			FrameResources& resources = m_FrameResources.emplace_back();

			resources.VertexBuffer = VertexBuffer::Create(sizeof(TextVertex) * 4 * m_RendererLimits.MaxQuadCount, GPUBufferUsage::Static);
			As<VulkanVertexBuffer>(resources.VertexBuffer)->GetBuffer().EnsureAllocated(); // HACk: To avoid binding NULL buffer
		}
	}

	TextPass::~TextPass()
	{
		FLARE_PROFILE_FUNCTION();

		for (FrameResources& resources : m_FrameResources)
			ReleaseDescriptorSets(resources.UsedSets);
	}

	void TextPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const Renderer2DFrameData& submition = context.GetSceneSubmition().Renderer2DSubmition;

		ReleaseDescriptorSets(frameResources.UsedSets);

		if (submition.TextQuadCount > 0)
		{
			frameResources.VertexBuffer->SetData(MemorySpan(submition.TextVertices.data(), submition.TextQuadCount * 4), 0, commandBuffer);
		}

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
				Renderer::GetCameraDescriptorSetPool()->GetLayout(),
				m_DescriptorSetPool->GetLayout()
			};

			m_TextPipeline = CreateRef<VulkanPipeline>(specificaionts,
				Span<Ref<const DescriptorSetLayout>>(layouts, 2),
				Span<ShaderPushConstantsRange>());
		}
	}

	void TextPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<FrameBuffer> renderTarget = context.GetRenderTarget();

		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const Renderer2DFrameData& submition = context.GetSceneSubmition().Renderer2DSubmition;

		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f), (glm::vec2)renderTarget->GetSize()));

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);
		
		for (const auto& batch : submition.TextBatches)
		{
			if (batch.Count == 0)
				continue;

			FlushBatch(batch, commandBuffer);
		}
	}

	void TextPass::FlushBatch(const TextBatch& batch, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		Ref<DescriptorSet> set = m_DescriptorSetPool->AllocateSet();
		set->SetDebugName("TextDescriptorSet");
		set->WriteImage(batch.Font->GetAtlas(), 0);
		set->FlushWrites();

		frameResources.UsedSets.push_back(set);

		commandBuffer->SetGlobalDescriptorSet(set, 1);
		commandBuffer->BindPipeline(m_TextPipeline);

		commandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>)frameResources.VertexBuffer), 0);
		commandBuffer->BindIndexBuffer(m_IndexBuffer);
		commandBuffer->DrawIndexed(batch.Start * 6, batch.Count * 6, 0, 0, 1);
	}

	void TextPass::ReleaseDescriptorSets(std::vector<Ref<DescriptorSet>>& sets)
	{
		FLARE_PROFILE_FUNCTION();
		for (const auto& set : sets)
		{
			m_DescriptorSetPool->ReleaseSet(set);
		}

		sets.clear();
	}
}
