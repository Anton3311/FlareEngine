#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Flare
{
	class VulkanFrameBuffer : public FrameBuffer
	{
	public:
		VulkanFrameBuffer(const FrameBufferSpecifications& specifications);
		VulkanFrameBuffer(uint32_t width, uint32_t height, const Ref<VulkanRenderPass>& compatibleRenderPass, const Span<VkImageView>& imageViews);
		~VulkanFrameBuffer();

		void Bind() override;
		void Unbind() override;
		void Resize(uint32_t width, uint32_t height) override;
		void* GetColorAttachmentRendererId(uint32_t attachmentIndex) const override;
		uint32_t GetAttachmentsCount() const override;
		void ClearAttachment(uint32_t index, const void* value) override;
		void ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput) override;
		void Blit(const Ref<FrameBuffer>& source, uint32_t destinationAttachment, uint32_t sourceAttachment) override;
		void BindAttachmentTexture(uint32_t attachment, uint32_t slot) override;
		void SetWriteMask(FrameBufferAttachmentsMask mask) override;
		FrameBufferAttachmentsMask GetWriteMask() override;
		const FrameBufferSpecifications& GetSpecifications() const override;

		glm::uvec2 GetSize() const { return glm::uvec2(m_Specifications.Width, m_Specifications.Height); }

		inline VkFramebuffer GetHandle() const { return m_FrameBuffer; }
		inline VkImageView GetAttachmentImageView(uint32_t attachment) const { return m_AttachmentsImageViews[attachment]; }
		inline VkImage GetAttachmentImage(uint32_t attachment) const { return m_AttachmentsImages[attachment]; }
		inline VkSampler GetDefaultAttachmentSampler(uint32_t attachment) const { return m_DefaultSamplers[attachment]; }
	private:
		void Create();
		void CreateImages();
		void ReleaseImages();
	private:
		bool m_OwnsImageViews = true;

		FrameBufferSpecifications m_Specifications;

		VkFramebuffer m_FrameBuffer = VK_NULL_HANDLE;

		std::vector<VkImage> m_AttachmentsImages;
		std::vector<VkImageView> m_AttachmentsImageViews;
		std::vector<VkDeviceMemory> m_AttachmentImagesMemory;
		std::vector<VkSampler> m_DefaultSamplers;

		Ref<VulkanRenderPass> m_CompatibleRenderPass = nullptr;
	};
}
