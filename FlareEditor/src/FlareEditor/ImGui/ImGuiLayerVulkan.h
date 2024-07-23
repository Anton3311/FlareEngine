#pragma once

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class Texture;
	class ImGuiLayerVulkan : public ImGuiLayer
	{
	public:
		void InitializeRenderer() override;
		void ShutdownRenderer() override;
		void InitializeFonts() override;

		void Begin() override;
		void End() override;

		void RenderCurrentWindow() override;
		void RenderWindows() override;
		void UpdateWindows() override;

		ImTextureID GetTextureId(const Ref<const Texture>& texture) override;
		ImTextureID GetFrameBufferAttachmentId(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachment) override;

		inline VkDescriptorSet GetFontsTextureDescrptor() const { return m_FontsTextureDescriptor; }
		inline VkPipeline GetPipeline() const { return m_Pipeline; }
		inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
	private:
		ImTextureID GetImageId(VkImageView imageView, VkSampler defaultSampler);

		void CreatePipeline();
		void CreatePipelineLayout();
		void CreateDescriptorSetLayout();
		void CreateFontsTexture();
	private:
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
		std::unordered_map<uint64_t, VkDescriptorSet> m_ImageToDescriptor;

		VkDescriptorSet m_FontsTextureDescriptor = VK_NULL_HANDLE;

		Ref<Texture> m_FontsTexture = nullptr;

		VkPipeline m_Pipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;
	};
}
