#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

namespace Flare
{
	class DescriptorSet;
	class GPUBuffer;
	class Material;

	class SpotLightShadowPass : public RenderGraphPass
	{
	public:
		SpotLightShadowPass(RenderGraphTextureId shadowMap, Ref<Material> perspectiveDepthOnly);
		~SpotLightShadowPass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		struct FrameResources
		{
			Ref<DescriptorSet> CameraDescriptorSet = nullptr;
			Ref<GPUBuffer> CameraBuffer = nullptr;
		};

		bool m_HasSpotLight = false;
		RenderGraphTextureId m_ShadowMap;
		std::vector<FrameResources> m_FrameResources;
		Ref<Material> m_PerspectiveDepthOnly = nullptr;
	};
}
