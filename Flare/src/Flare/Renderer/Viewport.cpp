#include "Viewport.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Renderer/Passes/ShadowPass.h"

namespace Flare
{
	Viewport::Viewport()
		: Graph(*this)
	{
		FLARE_PROFILE_FUNCTION();
		GlobalResources.CameraBuffer = UniformBuffer::Create(sizeof(RenderView));
		GlobalResources.LightBuffer = UniformBuffer::Create(sizeof(LightData));
		GlobalResources.ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowPass::ShadowData));
		GlobalResources.PointLightsBuffer = ShaderStorageBuffer::Create(16 * sizeof(PointLightData));
		GlobalResources.SpotLightsBuffer = ShaderStorageBuffer::Create(16 * sizeof(SpotLightData));

		GlobalResources.CameraDescriptorSet = Renderer::GetCameraDescriptorSetPool()->AllocateSet();
		GlobalResources.CameraDescriptorSet->WriteUniformBuffer(GlobalResources.CameraBuffer, 0);
		GlobalResources.CameraDescriptorSet->FlushWrites();

		GlobalResources.GlobalDescriptorSet = Renderer::GetGlobalDescriptorSetPool()->AllocateSet();
		GlobalResources.GlobalDescriptorSetWithoutShadows = Renderer::GetGlobalDescriptorSetPool()->AllocateSet();
	}

	Viewport::~Viewport()
	{
		FLARE_PROFILE_FUNCTION();
		Renderer::GetCameraDescriptorSetPool()->ReleaseSet(GlobalResources.CameraDescriptorSet);
		Renderer::GetGlobalDescriptorSetPool()->ReleaseSet(GlobalResources.GlobalDescriptorSet);
		Renderer::GetGlobalDescriptorSetPool()->ReleaseSet(GlobalResources.GlobalDescriptorSetWithoutShadows);
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

		SetupGlobalDescriptorSet(GlobalResources.GlobalDescriptorSet);
		SetupGlobalDescriptorSet(GlobalResources.GlobalDescriptorSetWithoutShadows);
	}

	void Viewport::OnBuildRenderGraph()
	{
		FLARE_PROFILE_FUNCTION();

		ColorTextureId = Graph.CreateTexture(m_ColorTextureFormat, "Color");
		NormalsTextureId = Graph.CreateTexture(m_NormalsTextureFormat, "Normals");
		DepthTextureId = Graph.CreateTexture(m_DepthTextureFormat, "Depth");

		ExternalRenderGraphResource colorTextureResource{};
		colorTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
		colorTextureResource.FinalLayout = ImageLayout::ReadOnly;
		colorTextureResource.Texture = ColorTextureId;

		ExternalRenderGraphResource normalsTextureResource{};
		normalsTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
		normalsTextureResource.FinalLayout = ImageLayout::ReadOnly;
		normalsTextureResource.Texture = NormalsTextureId;

		ExternalRenderGraphResource depthTextureResource{};
		depthTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
		depthTextureResource.FinalLayout = ImageLayout::ReadOnly;
		depthTextureResource.Texture = DepthTextureId;

		Graph.AddExternalResource(colorTextureResource);
		Graph.AddExternalResource(normalsTextureResource);
		Graph.AddExternalResource(depthTextureResource);
	}

	void Viewport::PrepareViewport()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_ShouldResizeRenderGraphTextures)
		{
			Graph.OnViewportResize();
			m_ShouldResizeRenderGraphTextures = false;
		}

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		const auto& resourceManager = Graph.GetResourceManager();

		commandBuffer->ClearColor(resourceManager.GetTexture(ColorTextureId), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		commandBuffer->ClearColor(resourceManager.GetTexture(NormalsTextureId), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		commandBuffer->ClearDepth(resourceManager.GetTexture(DepthTextureId), 1.0f);
	}

	void Viewport::SetPostProcessingEnabled(bool enabled)
	{
		if (m_PostProcessingEnabled == enabled)
			return;

		m_PostProcessingEnabled = enabled;
		Graph.SetNeedsRebuilding();
	}

	void Viewport::SetShadowMappingEnabled(bool enabled)
	{
		if (m_ShadowMappingEnabled == enabled)
			return;

		m_ShadowMappingEnabled = enabled;
		Graph.SetNeedsRebuilding();
	}

	void Viewport::SetDebugRenderingEnabled(bool enabled)
	{
		if (m_DebugRenderingEnabled == enabled)
			return;

		m_DebugRenderingEnabled = enabled;
		Graph.SetNeedsRebuilding();
	}

	void Viewport::SetupGlobalDescriptorSet(Ref<DescriptorSet> set)
	{
		FLARE_PROFILE_FUNCTION();
		set->WriteUniformBuffer(GlobalResources.ShadowDataBuffer, 0);
		set->WriteUniformBuffer(GlobalResources.LightBuffer, 1);
		set->WriteStorageBuffer(GlobalResources.PointLightsBuffer, 2);
		set->WriteStorageBuffer(GlobalResources.SpotLightsBuffer, 3);
		set->FlushWrites();
	}
}
