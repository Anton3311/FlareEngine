#include "PCH.h"

#include "TextPass.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "Flare/Renderer2D/Renderer2DFrameData.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"

namespace Flare
{
	TextPass::TextPass(const Renderer2DLimits& limits, Ref<GPUBuffer> indexBuffer, Ref<Shader> textShader, Ref<DescriptorSetPool> descriptorSetPool)
		: m_RendererLimits(limits), m_IndexBuffer(indexBuffer), m_TextShader(textShader), m_DescriptorSetPool(descriptorSetPool)
	{
		FLARE_PROFILE_FUNCTION();
		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			FrameResources& resources = m_FrameResources.emplace_back();

			resources.VertexBuffer = GPUBuffer::CreateVertexBuffer(sizeof(TextVertex) * 4 * m_RendererLimits.MaxQuadCount, GPUBufferMemoryType::Static);
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
			PipelineSpecifications specifications{};
			specifications.Shader = m_TextShader;
			specifications.Culling = CullingMode::Back;
			specifications.DepthTest = false;
			specifications.DepthWrite = false;
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 0, 1, ShaderDataType::Float4 }, // Color
				{ 0, 2, ShaderDataType::Float2 }, // UV
				{ 0, 3, ShaderDataType::Int }, // Entity index
			});

			Ref<const DescriptorSetLayout> layouts[] =
			{
				Renderer::GetCameraDescriptorSetPool()->GetLayout(),
				m_DescriptorSetPool->GetLayout()
			};

			m_TextPipeline = Ref<VulkanPipeline>::New(specifications,
				Span<Ref<const DescriptorSetLayout>>(layouts, 2),
				Span<ShaderPushConstantsRange>());
		}
	}

	void TextPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const Renderer2DFrameData& submition = context.GetSceneSubmition().Renderer2DSubmition;

		const ViewportGlobalResources* viewportResources = context.RenderWorld.TryGetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		FLARE_CORE_ASSERT(viewportResources);

		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().CameraDescriptorSet, 0);
		
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

		Ref<const GPUBuffer> vertexBuffer = (Ref<const GPUBuffer>)frameResources.VertexBuffer;

		commandBuffer->BindVertexBuffers(Span<Ref<const GPUBuffer>>(vertexBuffer), 0);
		commandBuffer->BindIndexBuffer(m_IndexBuffer, IndexFormat::UInt32);
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
