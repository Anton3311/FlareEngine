#pragma once

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Pipeline.h"

namespace Flare
{
	class VulkanPipeline;
	class VulkanDescriptorSet;
	class VulkanRenderPass;

	class FLARE_API VulkanMaterial : public Material
	{
	public:
		virtual ~VulkanMaterial();

		virtual void SetShader(const Ref<Shader>& shader) override;

		Ref<VulkanPipeline> GetPipeline(const Ref<VulkanRenderPass>& renderPass);
		Ref<VulkanDescriptorSet> GetDescriptorSet() const { return m_Set; }

		void UpdateDescriptorSet();
	private:
		void ReleaseDescriptorSet();
	private:
		Ref<VulkanPipeline> m_Pipeline = nullptr;
		Ref<VulkanDescriptorSet> m_Set = nullptr;
	};
}
