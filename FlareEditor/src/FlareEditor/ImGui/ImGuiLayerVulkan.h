#pragma once

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class ImGuiLayerVulkan : public ImGuiLayer
	{
	public:
		void InitializeRenderer() override;
		void ShutdownRenderer() override;
		void InitializeFonts() override;

		void Begin() override;
		void End() override;

		void RenderCurrentWindow() override;
		void UpdateWindows() override;

		ImTextureID GetTextureId(const Ref<const Texture>& texture) override;
		ImTextureID GetFrameBufferAttachmentId(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachment) override;
	private:
		ImTextureID GetImageId(VkImageView imageView, VkSampler defaultSampler);
	private:
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		std::unordered_map<uint64_t, VkDescriptorSet> m_ImageToDescriptor;
	};
}
