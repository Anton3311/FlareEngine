#include "PCH.h"

#include "RendererComponents.h"

#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Passes/ShadowPass.h"
#include "Flare/Renderer/Passes/SpotLightShadowPass.h"

namespace Flare
{
	FLARE_IMPL_COMPONENT(Viewport);
	FLARE_IMPL_COMPONENT(ViewportGlobalResources);
	FLARE_IMPL_COMPONENT(ViewportColorOutput);
	FLARE_IMPL_COMPONENT(ViewportDepthOutput);
	FLARE_IMPL_COMPONENT(ViewportRenderGraph);

	ViewportFrameResources::~ViewportFrameResources()
	{
		if (CameraDescriptorSet)
			Renderer::GetCameraDescriptorSetPool()->ReleaseSet(CameraDescriptorSet);
		if (GlobalDescriptorSet)
			Renderer::GetGlobalDescriptorSetPool()->ReleaseSet(GlobalDescriptorSet);
		if (GlobalDescriptorSetWithoutShadows)
			Renderer::GetGlobalDescriptorSetPool()->ReleaseSet(GlobalDescriptorSetWithoutShadows);
	}

	void ViewportGlobalResources::CreateResources()
	{
		FLARE_CORE_ASSERT(FrameResources.size() == 0);
		FrameResources.resize(GraphicsContext::GetInstance().GetFrameInFlightCount());

		for (size_t frameIndex = 0; frameIndex < FrameResources.size(); frameIndex++)
		{
			ViewportFrameResources& frameResources = FrameResources[frameIndex];

			frameResources.CameraBuffer = GPUBuffer::CreateUniformBuffer(sizeof(RenderView));
			frameResources.LightBuffer = GPUBuffer::CreateUniformBuffer(sizeof(LightData));
			frameResources.ShadowDataBuffer = GPUBuffer::CreateUniformBuffer(sizeof(ShadowPass::ShadowData));
			frameResources.PointLightsBuffer = GPUBuffer::CreateStorageBuffer(16 * sizeof(PointLightData), GPUBufferMemoryType::Static);
			frameResources.SpotLightsBuffer = GPUBuffer::CreateStorageBuffer(16 * sizeof(SpotLightData), GPUBufferMemoryType::Static);
			frameResources.SpotLightShadowDataBuffer = GPUBuffer::CreateStorageBuffer(sizeof(SpotLightShadowsEntry) * 8 + 16, GPUBufferMemoryType::Dynamic);

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

	const ViewportFrameResources& ViewportGlobalResources::GetCurrentFrameResources() const
	{
		return FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
	}

	void ViewportGlobalResources::SetupGlobalDescriptorSet(const ViewportFrameResources& frameResources, Ref<DescriptorSet> set) const
	{
		FLARE_PROFILE_FUNCTION();
		set->WriteUniformBuffer(frameResources.ShadowDataBuffer, 0);
		set->WriteUniformBuffer(frameResources.LightBuffer, 1);
		set->WriteStorageBuffer(frameResources.PointLightsBuffer, 2);
		set->WriteStorageBuffer(frameResources.SpotLightsBuffer, 3);
		set->WriteStorageBuffer(frameResources.SpotLightShadowDataBuffer, 14);
		set->FlushWrites();
	}

	FLARE_IMPL_COMPONENT(AOConfiguration);
}