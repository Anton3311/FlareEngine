#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetPool;
	class Material;
	class ShaderStorageBuffer;
	class Texture;

	struct DecalSubmitionData
	{
		Ref<const Material> Material = nullptr;
		glm::vec4 PackedTransform[4] = { glm::vec4(0.0f) };
	};

	class DecalsPass : public RenderGraphPass
	{
	public:
		DecalsPass(const std::vector<DecalSubmitionData>& submitedDecals, Ref<DescriptorSetPool> decalDescriptorPool, Ref<Texture> depthTexture);

		~DecalsPass();
	public:
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};

		const std::vector<DecalSubmitionData>& m_SubmitedDecals;

		Ref<Texture> m_DepthTexture = nullptr;

		std::vector<InstanceData> m_InstanceData;
		Ref<ShaderStorageBuffer> m_InstanceBuffer = nullptr;
		Ref<DescriptorSet> m_InstanceDataDescriptor = nullptr;

		Ref<DescriptorSet> m_DecalSet = nullptr;
		Ref<DescriptorSetPool> m_DecalDescriptorPool = nullptr;
	};
}
