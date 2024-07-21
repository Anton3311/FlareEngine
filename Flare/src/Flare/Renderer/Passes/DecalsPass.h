#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetPool;
	class Material;
	class ShaderStorageBuffer;
	class Texture;

	class DecalsPass : public RenderGraphPass
	{
	public:
		DecalsPass(Ref<DescriptorSetPool> decalDescriptorPool, RenderGraphTextureId depthTexture);

		~DecalsPass();
	public:
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};

		bool m_ShouldUpdateDescriptorSet = false;
		RenderGraphTextureId m_DepthTexture;

		std::vector<InstanceData> m_InstanceData;
		Ref<ShaderStorageBuffer> m_InstanceBuffer = nullptr;
		Ref<DescriptorSet> m_InstanceDataDescriptor = nullptr;

		Ref<DescriptorSet> m_DecalSet = nullptr;
		Ref<DescriptorSetPool> m_DecalDescriptorPool = nullptr;
	};
}
