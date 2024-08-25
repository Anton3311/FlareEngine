#pragma once

#include "FlareCore/Collections/Span.h"
#include "Flare/Renderer/ComputeShader.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetPool;
	class VulkanComputeShader : public ComputeShader
	{
	public:
		VulkanComputeShader();
		~VulkanComputeShader();

		const Ref<const ComputeShaderMetadata>& GetMetadata() const override;
		void Load() override;
		bool IsLoaded() const override;

		inline VkPipelineLayout GetPipelineLayoutHandle() const { return m_PipelineLayout; }
		inline VkPipeline GetPipeline() const { return m_Pipeline; }
		inline Ref<DescriptorSetPool> GetSetPool() const { return m_SetPool; }
	private:
		void Release();

		void CreatePipelineLayout();
		void CreatePipeline();
	private:
		Ref<const ComputeShaderMetadata> m_Metadata = nullptr;
		bool m_IsLoaded = false;

		VkShaderModule m_Module = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		Ref<DescriptorSetPool> m_SetPool = nullptr;
	};
}
