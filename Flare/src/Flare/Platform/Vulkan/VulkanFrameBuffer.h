#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Flare
{
	class VulkanFrameBuffer : public FrameBuffer
	{
	public:
		VulkanFrameBuffer(const FrameBufferSpecifications& specifications);
		VulkanFrameBuffer(Span<Ref<Texture>> attachmentTextures);
		VulkanFrameBuffer(uint32_t width, uint32_t height,
			const Ref<VulkanRenderPass>& compatibleRenderPass,
			const Span<Ref<Texture>>& attachmentTextures,
			bool transitionToDefaultLayout);
		
		~VulkanFrameBuffer();

		void Resize(uint32_t width, uint32_t height) override;

		uint32_t GetAttachmentsCount() const override;
		uint32_t GetColorAttachmentsCount() const override;
		std::optional<uint32_t> GetDepthAttachmentIndex() const override;

		Ref<Texture> GetAttachment(uint32_t index) const override;

		const FrameBufferSpecifications& GetSpecifications() const override;
		void SetDebugName(std::string_view debugName) override;
		const std::string& GetDebugName() const override;

		glm::uvec2 GetSize() const { return glm::uvec2(m_Specifications.Width, m_Specifications.Height); }

		inline VkFramebuffer GetHandle() const { return m_FrameBuffer; }
		inline VkImageView GetAttachmentImageView(uint32_t attachment) const { return m_Attachments[attachment]->GetImageViewHandle(); }
		inline VkImage GetAttachmentImage(uint32_t attachment) const { return m_Attachments[attachment]->GetImageHandle(); }
		inline VkSampler GetDefaultAttachmentSampler(uint32_t attachment) const { return m_Attachments[attachment]->GetDefaultSampler(); }

		inline Ref<VulkanRenderPass> GetCompatibleRenderPass() const { return m_CompatibleRenderPass; }
	private:
		void Create();
		void CreateImages();
		void ReleaseImages();
		void TransitionToDefaultImageLayout();
	private:
		std::string m_DebugName;
		bool m_ShouldTransitionToDefaultLayout = true;
		FrameBufferSpecifications m_Specifications;

		VkFramebuffer m_FrameBuffer = VK_NULL_HANDLE;

		std::optional<uint32_t> m_DepthAttachmentIndex;
		std::vector<Ref<VulkanTexture>> m_Attachments;

		Ref<VulkanRenderPass> m_CompatibleRenderPass = nullptr;
	};
}
