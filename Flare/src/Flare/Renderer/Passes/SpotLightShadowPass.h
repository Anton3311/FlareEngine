#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

namespace Flare
{
	class DescriptorSet;
	class GPUBuffer;
	class Material;

	struct SpotLightShadowsEntry
	{
		glm::vec2 UVScale;
		glm::vec2 UVTranslation;
		glm::mat4 Projection;
	};

	struct SpotLightShadowsSpecifications
	{
		uint32_t TileSize = 256;
		glm::uvec2 TileCount = glm::uvec2(2, 2);
	};

	class SpotLightShadowPass : public RenderGraphPass
	{
	public:
		SpotLightShadowPass(RenderGraphTextureId shadowMap,
			Ref<Material> perspectiveDepthOnly,
			const SpotLightShadowsSpecifications& specifications);
		~SpotLightShadowPass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		struct FrameResources
		{
			Ref<DescriptorSet> CameraDescriptorSet = nullptr;
			Ref<GPUBuffer> CameraBuffer = nullptr;
		};

		SpotLightShadowsSpecifications m_Specifications;

		bool m_HasSpotLight = false;
		RenderGraphTextureId m_ShadowMap;
		std::vector<FrameResources> m_FrameResources;
		Ref<Material> m_PerspectiveDepthOnly = nullptr;
	};
}
