#include "Viewport.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Renderer/Passes/ShadowPass.h"

namespace Flare
{
	Viewport::Viewport()
	{
		FLARE_PROFILE_FUNCTION();

		m_RenderGraph = RenderGraph::Create(*this);

		m_GlobalResources.FrameResources.resize(GraphicsContext::GetInstance().GetFrameInFlightCount());

		for (size_t frameIndex = 0; frameIndex < m_GlobalResources.FrameResources.size(); frameIndex++)
		{
			ViewportFrameResources& frameResources = m_GlobalResources.FrameResources[frameIndex];

			frameResources.CameraBuffer = GPUBuffer::CreateUniformBuffer(sizeof(RenderView));
			frameResources.LightBuffer = GPUBuffer::CreateUniformBuffer(sizeof(LightData));
			frameResources.ShadowDataBuffer = GPUBuffer::CreateUniformBuffer(sizeof(ShadowPass::ShadowData));
			frameResources.PointLightsBuffer = GPUBuffer::CreateStorageBuffer(16 * sizeof(PointLightData), GPUBufferMemoryType::Static);
			frameResources.SpotLightsBuffer = GPUBuffer::CreateStorageBuffer(16 * sizeof(SpotLightData), GPUBufferMemoryType::Static);

			frameResources.CameraDescriptorSet = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
			frameResources.CameraDescriptorSet->WriteUniformBuffer(frameResources.CameraBuffer, 0);
			frameResources.CameraDescriptorSet->FlushWrites();

			frameResources.GlobalDescriptorSet = Renderer::GetGlobalDescriptorSetPool()->AllocateSet();
			frameResources.GlobalDescriptorSetWithoutShadows = Renderer::GetGlobalDescriptorSetPool()->AllocateSet();
			
			frameResources.CameraBuffer->SetDebugName(fmt::format("CameraSet.#{}", frameIndex));
			frameResources.GlobalDescriptorSet->SetDebugName(fmt::format("GlobalSet.#{}", frameIndex));
			frameResources.GlobalDescriptorSetWithoutShadows->SetDebugName(fmt::format("GlobalSetWithoutShadows.#{}", frameIndex));
		}
	}

	Viewport::~Viewport()
	{
		FLARE_PROFILE_FUNCTION();

		for (const ViewportFrameResources& frameResources : m_GlobalResources.FrameResources)
		{
			Renderer::GetCameraDescriptorSetPool()->ReleaseSet(frameResources.CameraDescriptorSet);
			Renderer::GetGlobalDescriptorSetPool()->ReleaseSet(frameResources.GlobalDescriptorSet);
			Renderer::GetGlobalDescriptorSetPool()->ReleaseSet(frameResources.GlobalDescriptorSetWithoutShadows);
		}
	}

	void Viewport::Resize(glm::ivec2 position, glm::ivec2 size)
	{
		if (m_Size != size)
			m_ShouldResizeRenderGraphTextures = true;

		m_Position = position;
		m_Size = size;
	}

	void Viewport::UpdateGlobalDescriptorSets()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_CurrentFrameResources);

		SetupGlobalDescriptorSet(*m_CurrentFrameResources, m_CurrentFrameResources->GlobalDescriptorSet);
		SetupGlobalDescriptorSet(*m_CurrentFrameResources, m_CurrentFrameResources->GlobalDescriptorSetWithoutShadows);
	}

	void Viewport::OnBuildRenderGraph()
	{
		FLARE_PROFILE_FUNCTION();

		ColorTextureId = m_RenderGraph->CreateTexture(m_ColorTextureFormat, "Color");
		DepthTextureId = m_RenderGraph->CreateTexture(m_DepthTextureFormat, "Depth");

		ExternalRenderGraphResource colorTextureResource{};
		colorTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
		colorTextureResource.FinalLayout = ImageLayout::ReadOnly;
		colorTextureResource.Texture = ColorTextureId;

		ExternalRenderGraphResource depthTextureResource{};
		depthTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
		depthTextureResource.FinalLayout = ImageLayout::ReadOnly;
		depthTextureResource.Texture = DepthTextureId;

		m_RenderGraph->AddExternalResource(colorTextureResource);
		m_RenderGraph->AddExternalResource(depthTextureResource);
	}

	void Viewport::PrepareViewport()
	{
		FLARE_PROFILE_FUNCTION();

		m_CurrentFrameResources = &m_GlobalResources.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];

		m_RenderGraph->Prepare();

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		const auto& resourceManager = m_RenderGraph->GetResourceManager();

		commandBuffer->ClearColor(resourceManager.GetTexture(ColorTextureId), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		commandBuffer->ClearDepth(resourceManager.GetTexture(DepthTextureId), 1.0f);
	}

	void Viewport::SetPostProcessingEnabled(bool enabled)
	{
		if (m_PostProcessingEnabled == enabled)
			return;

		m_PostProcessingEnabled = enabled;
		m_RenderGraph->SetNeedsRebuilding();
	}

	void Viewport::SetShadowMappingEnabled(bool enabled)
	{
		if (m_ShadowMappingEnabled == enabled)
			return;

		m_ShadowMappingEnabled = enabled;
		m_RenderGraph->SetNeedsRebuilding();
	}

	void Viewport::SetDebugRenderingEnabled(bool enabled)
	{
		if (m_DebugRenderingEnabled == enabled)
			return;

		m_DebugRenderingEnabled = enabled;
		m_RenderGraph->SetNeedsRebuilding();
	}

	const ViewportFrameResources& Viewport::GetFrameResources() const
	{
		return m_GlobalResources.FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
	}

	void Viewport::SetupGlobalDescriptorSet(const ViewportFrameResources& frameResources, Ref<DescriptorSet> set)
	{
		FLARE_PROFILE_FUNCTION();
		set->WriteUniformBuffer(frameResources.ShadowDataBuffer, 0);
		set->WriteUniformBuffer(frameResources.LightBuffer, 1);
		set->WriteStorageBuffer(frameResources.PointLightsBuffer, 2);
		set->WriteStorageBuffer(frameResources.SpotLightsBuffer, 3);
		set->FlushWrites();
	}
}
