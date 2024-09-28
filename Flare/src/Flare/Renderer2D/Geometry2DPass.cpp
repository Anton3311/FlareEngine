#include "Geometry2DPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Math/Math.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Renderer2D/Renderer2DFrameData.h"

namespace Flare
{
	Geometry2DPass::Geometry2DPass(const Renderer2DLimits& limits,
		Ref<GPUBuffer> indexBuffer,
		Ref<Material> defaultMaterial,
		Ref<DescriptorSetPool> quadsDescriptorSetPool)
		: m_RendererLimits(limits), m_IndexBuffer(indexBuffer), m_DefaultMaterial(defaultMaterial), m_QuadsDescriptorSetPool(quadsDescriptorSetPool)
	{
		FLARE_PROFILE_FUNCTION();
		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			FrameResources& resources = m_FrameResources.emplace_back();
			resources.VertexBuffer = GPUBuffer::CreateVertexBuffer(sizeof(QuadVertex) * 4 * m_RendererLimits.MaxQuadCount, GPUBufferMemoryType::Static);
		}
	}

	Geometry2DPass::~Geometry2DPass()
	{
		for (FrameResources& resources : m_FrameResources)
			ReleaseDescriptorSets(resources.UsedSets);
	}

	void Geometry2DPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const Renderer2DFrameData& submition = context.GetSceneSubmition().Renderer2DSubmition;
		FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		if (submition.QuadCount > 0)
		{
			frameResources.VertexBuffer->SetData(MemorySpan(const_cast<QuadVertex*>(submition.QuadVertices.data()), submition.QuadCount * 4), 0, commandBuffer);
		}

		ReleaseDescriptorSets(frameResources.UsedSets);
	}

	void Geometry2DPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const Renderer2DFrameData& submition = context.GetSceneSubmition().Renderer2DSubmition;
		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);
		commandBuffer->BindVertexBuffers(Span((Ref<const GPUBuffer>*)&frameResources.VertexBuffer, 1), 0);
		commandBuffer->BindIndexBuffer(m_IndexBuffer, IndexFormat::UInt32);

		for (const auto& batch : submition.QuadBatches)
		{
			if (batch.Count == 0)
				continue;

			FlushBatch(context, batch, commandBuffer);
		}
	}

	void Geometry2DPass::ReleaseDescriptorSets(std::vector<Ref<DescriptorSet>>& sets)
	{
		FLARE_PROFILE_FUNCTION();

		for (auto set : sets)
		{
			m_QuadsDescriptorSetPool->ReleaseSet(set);
		}

		sets.clear();
	}

	void Geometry2DPass::FlushBatch(const RenderGraphContext& context, const QuadsBatch& batch, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		Ref<DescriptorSet> descriptorSet = m_QuadsDescriptorSetPool->AllocateSet();
		descriptorSet->SetDebugName("QuadsDescriptorSet");
		descriptorSet->WriteImages(Span((Ref<const Texture>*)batch.Textures, Renderer2DLimits::MaxTexturesCount), 0, 0);
		descriptorSet->FlushWrites();

		frameResources.UsedSets.push_back(descriptorSet);

		commandBuffer->SetGlobalDescriptorSet(descriptorSet, 1);
		commandBuffer->ApplyMaterial(batch.Material);

		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->DrawIndexed(batch.Start * 6, batch.Count * 6, 0, 0, 1);
	}
}
