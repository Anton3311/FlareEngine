#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetPool;
	class GPUBuffer;
	class Material;
	class Texture;

	class DecalsPass : public RenderGraphPass
	{
	public:
		DecalsPass(Ref<DescriptorSetPool> decalDescriptorPool, RenderGraphTextureId depthTexture);

		~DecalsPass();
	public:
		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};

		struct FrameResources
		{
			Ref<GPUBuffer> InstanceBuffer = nullptr;
			Ref<DescriptorSet> InstanceBufferDescriptor = nullptr;
			Ref<DescriptorSet> DecalSet = nullptr;
		};

		bool m_ShouldUpdateDescriptorSet = false;
		RenderGraphTextureId m_DepthTexture;

		std::vector<FrameResources> m_FrameResources;
		std::vector<InstanceData> m_InstanceData;

		Ref<DescriptorSetPool> m_DecalDescriptorPool = nullptr;
	};
}
