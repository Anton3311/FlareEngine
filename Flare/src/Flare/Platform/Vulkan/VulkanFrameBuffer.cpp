#include "VulkanFrameBuffer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VkFormat FrameBufferAttachmentFormatToVulkanFormat(FrameBufferTextureFormat format)
	{
		switch (format)
		{
		case FrameBufferTextureFormat::RGB8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case FrameBufferTextureFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case FrameBufferTextureFormat::Depth24Stencil8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case FrameBufferTextureFormat::RedInteger:
			return VK_FORMAT_R32_SINT;
		case FrameBufferTextureFormat::RF32:
			return VK_FORMAT_R32_SFLOAT;
		case FrameBufferTextureFormat::R11G11B10:
			return VK_FORMAT_R8G8B8A8_UNORM;
		}

		FLARE_CORE_ASSERT(false);
		return VK_FORMAT_UNDEFINED;
	}

	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferSpecifications& specifications)
		: m_OwnsImageViews(true), m_Specifications(specifications)
	{
		FLARE_CORE_ASSERT(m_Specifications.Width > 0 && m_Specifications.Height > 0);

		m_AttachmentsImages.resize(specifications.Attachments.size());
		m_AttachmentsImageViews.resize(specifications.Attachments.size());
		m_AttachmentAllocations.resize(specifications.Attachments.size());
		m_DefaultSamplers.resize(specifications.Attachments.size());

		CreateImages();
		Create();

		for (size_t i = 0; i < m_Specifications.Attachments.size(); i++)
		{
			const auto& attachment = m_Specifications.Attachments[i];

			VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			switch (attachment.Wrap)
			{
			case TextureWrap::Clamp:
				addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			case TextureWrap::Repeat:
				addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				break;
			}

			VkFilter filter = VK_FILTER_NEAREST;
			switch (attachment.Filtering)
			{
			case TextureFiltering::Closest:
				filter = VK_FILTER_NEAREST;
				break;
			case TextureFiltering::Linear:
				filter = VK_FILTER_LINEAR;
				break;
			}

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.addressModeU = addressMode;
			samplerInfo.addressModeV = addressMode;
			samplerInfo.addressModeW = addressMode;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.borderColor = {};
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.flags = 0;
			samplerInfo.magFilter = filter;
			samplerInfo.minFilter = filter;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;
			samplerInfo.mipLodBias = 0.0f;

			VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetInstance().GetDevice(), &samplerInfo, nullptr, &m_DefaultSamplers[i]));
		}
	}

	VulkanFrameBuffer::VulkanFrameBuffer(uint32_t width, uint32_t height, const Ref<VulkanRenderPass>& compatibleRenderPass, const Span<VkImageView>& imageViews)
		: m_OwnsImageViews(false), m_CompatibleRenderPass(compatibleRenderPass)
	{
		m_AttachmentsImageViews.assign(imageViews.begin(), imageViews.end());

		m_Specifications.Width = width;
		m_Specifications.Height = height;

		FLARE_CORE_ASSERT(m_Specifications.Width > 0 && m_Specifications.Height > 0);

		Create();
	}

	VulkanFrameBuffer::~VulkanFrameBuffer()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		ReleaseImages();

		for (VkSampler sampler : m_DefaultSamplers)
		{
			vkDestroySampler(VulkanContext::GetInstance().GetDevice(), sampler, nullptr);
		}

		vkDestroyFramebuffer(VulkanContext::GetInstance().GetDevice(), m_FrameBuffer, nullptr);
	}

	void VulkanFrameBuffer::Bind()
	{
	}

	void VulkanFrameBuffer::Unbind()
	{
	}

	void VulkanFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		FLARE_CORE_ASSERT(m_OwnsImageViews);

		ReleaseImages();

		vkDestroyFramebuffer(VulkanContext::GetInstance().GetDevice(), m_FrameBuffer, nullptr);

		m_Specifications.Width = width;
		m_Specifications.Height = height;

		CreateImages();
		Create();
	}

	uint32_t VulkanFrameBuffer::GetAttachmentsCount() const
	{
		return (uint32_t)m_Specifications.Attachments.size();
	}

	void VulkanFrameBuffer::ClearAttachment(uint32_t index, const void* value)
	{
	}

	void VulkanFrameBuffer::ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput)
	{
	}

	void VulkanFrameBuffer::Blit(const Ref<FrameBuffer>& source, uint32_t destinationAttachment, uint32_t sourceAttachment)
	{
	}

	void VulkanFrameBuffer::BindAttachmentTexture(uint32_t attachment, uint32_t slot)
	{
	}

	void VulkanFrameBuffer::SetWriteMask(FrameBufferAttachmentsMask mask)
	{
	}

	FrameBufferAttachmentsMask VulkanFrameBuffer::GetWriteMask()
	{
		return ~0;
	}

	const FrameBufferSpecifications& VulkanFrameBuffer::GetSpecifications() const
	{
		return m_Specifications;
	}

	void VulkanFrameBuffer::Create()
	{
		if (m_CompatibleRenderPass == nullptr)
		{
			std::vector<FrameBufferTextureFormat> formats;
			formats.reserve(m_Specifications.Attachments.size());

			for (const auto& attachment : m_Specifications.Attachments)
			{
				formats.push_back(attachment.Format);
			}

			m_CompatibleRenderPass = VulkanContext::GetInstance().FindOrCreateRenderPass(Span<FrameBufferTextureFormat>::FromVector(formats));
		}

		FLARE_CORE_ASSERT(m_Specifications.Width > 0 && m_Specifications.Height > 0);

		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.width = m_Specifications.Width;
		info.height = m_Specifications.Height;
		info.renderPass = m_CompatibleRenderPass->GetHandle();
		info.layers = 1;
		info.attachmentCount = (uint32_t)m_AttachmentsImageViews.size();
		info.pAttachments = m_AttachmentsImageViews.data();
		info.flags = 0;

		VK_CHECK_RESULT(vkCreateFramebuffer(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_FrameBuffer));

		Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
		for (size_t i = 0; i < m_AttachmentsImages.size(); i++)
		{
			if (IsDepthFormat(m_Specifications.Attachments[i].Format))
			{
				commandBuffer->TransitionDepthImageLayout(m_AttachmentsImages[i],
					m_Specifications.Attachments[i].Format == FrameBufferTextureFormat::Depth24Stencil8,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
			else
			{
				commandBuffer->TransitionImageLayout(m_AttachmentsImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
		VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
	}

	void VulkanFrameBuffer::CreateImages()
	{
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		for (size_t i = 0; i < m_Specifications.Attachments.size(); i++)
		{
			const auto& attachment = m_Specifications.Attachments[i];

			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.arrayLayers = 1;
			imageInfo.mipLevels = 1;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = m_Specifications.Width;
			imageInfo.extent.height = m_Specifications.Height;
			imageInfo.extent.depth = 1;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			if (IsDepthFormat(attachment.Format))
			{
				imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else
			{
				imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}

			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.format = FrameBufferAttachmentFormatToVulkanFormat(attachment.Format);
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

			VmaAllocationCreateInfo allocationInfo{};
			allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VK_CHECK_RESULT(vmaCreateImage(
				VulkanContext::GetInstance().GetMemoryAllocator(),
				&imageInfo, &allocationInfo,
				&m_AttachmentsImages[i],
				&m_AttachmentAllocations[i].Handle,
				&m_AttachmentAllocations[i].Info));

			VkImageViewCreateInfo imageViewInfo{};
			imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
			imageViewInfo.flags = 0;
			imageViewInfo.format = FrameBufferAttachmentFormatToVulkanFormat(attachment.Format);

			if (IsDepthFormat(attachment.Format))
			{
				imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			imageViewInfo.subresourceRange.baseArrayLayer = 0;
			imageViewInfo.subresourceRange.baseMipLevel = 0;
			imageViewInfo.subresourceRange.layerCount = 1;
			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.image = m_AttachmentsImages[i];

			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewInfo, nullptr, &m_AttachmentsImageViews[i]));
		}
	}

	void VulkanFrameBuffer::ReleaseImages()
	{
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		if (m_OwnsImageViews)
		{
			for (VkImageView& view : m_AttachmentsImageViews)
			{
				VulkanContext::GetInstance().NotifyImageViewDeletionHandler(view);

				vkDestroyImageView(device, view, nullptr);
				view = VK_NULL_HANDLE;
			}
		}

		for (size_t i = 0; i < m_AttachmentsImages.size(); i++)
		{
			vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_AttachmentAllocations[i].Handle);
			vkDestroyImage(device, m_AttachmentsImages[i], nullptr);

			m_AttachmentsImages[i] = VK_NULL_HANDLE;
			m_AttachmentAllocations[i] = {};
		}
	}
}
