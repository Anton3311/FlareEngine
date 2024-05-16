#include "VulkanFrameBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Texture.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

namespace Flare
{
	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferSpecifications& specifications)
		: m_Specifications(specifications), m_OwnsAttachmentTextures(true)
	{
		FLARE_CORE_ASSERT(m_Specifications.Width > 0 && m_Specifications.Height > 0);

		CreateImages();
		Create();
	}

	VulkanFrameBuffer::VulkanFrameBuffer(uint32_t width, uint32_t height, const Ref<VulkanRenderPass>& compatibleRenderPass, const Span<Ref<Texture>>& attachmentTextures)
		: m_CompatibleRenderPass(compatibleRenderPass), m_OwnsAttachmentTextures(false)
	{
		FLARE_CORE_ASSERT(attachmentTextures.GetSize() > 0);
		FLARE_CORE_ASSERT(width > 0 && height > 0);

		m_Specifications.Width = width;
		m_Specifications.Height = height;

		m_Attachments.reserve(attachmentTextures.GetSize());
		m_Specifications.Attachments.reserve(attachmentTextures.GetSize());
		for (const auto& attachment : attachmentTextures)
		{
			FLARE_CORE_ASSERT(attachment->GetWidth() == width && attachment->GetHeight() == height);
			FLARE_CORE_ASSERT(HAS_BIT(attachment->GetSpecifications().Usage, TextureUsage::RenderTarget));

			auto& attachmentSpecifications = m_Specifications.Attachments.emplace_back();
			attachmentSpecifications.Filtering = attachment->GetFiltering();
			attachmentSpecifications.Format = attachment->GetFormat();
			attachmentSpecifications.Wrap = attachment->GetSpecifications().Wrap;

			m_Attachments.push_back(As<VulkanTexture>(attachment));
		}

		Create();
	}

	VulkanFrameBuffer::~VulkanFrameBuffer()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		ReleaseImages();

		vkDestroyFramebuffer(VulkanContext::GetInstance().GetDevice(), m_FrameBuffer, nullptr);
	}

	void VulkanFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		FLARE_CORE_ASSERT(width > 0 && height > 0);

		for (const auto& attachment : m_Attachments)
			attachment->Resize(width, height);

		vkDestroyFramebuffer(VulkanContext::GetInstance().GetDevice(), m_FrameBuffer, nullptr);

		m_Specifications.Width = width;
		m_Specifications.Height = height;

		Create();
	}

	uint32_t VulkanFrameBuffer::GetAttachmentsCount() const
	{
		return (uint32_t)m_Specifications.Attachments.size();
	}

	uint32_t VulkanFrameBuffer::GetColorAttachmentsCount() const
	{
		if (m_DepthAttachmentIndex)
			return (uint32_t)m_Specifications.Attachments.size() - 1;

		return (uint32_t)m_Specifications.Attachments.size();
	}

	std::optional<uint32_t> VulkanFrameBuffer::GetDepthAttachmentIndex() const
	{
		return m_DepthAttachmentIndex;
	}

	Ref<Texture> VulkanFrameBuffer::GetAttachment(uint32_t index) const
	{
		FLARE_CORE_ASSERT((size_t)index < m_Attachments.size());
		return As<Texture>(m_Attachments[index]);
	}

	const FrameBufferSpecifications& VulkanFrameBuffer::GetSpecifications() const
	{
		return m_Specifications;
	}

	void VulkanFrameBuffer::Create()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Attachments.size() > 0);

		if (m_CompatibleRenderPass == nullptr)
		{
			std::vector<TextureFormat> formats;
			formats.reserve(m_Specifications.Attachments.size());

			for (const auto& attachment : m_Specifications.Attachments)
			{
				formats.push_back(attachment.Format);
			}

			m_CompatibleRenderPass = VulkanContext::GetInstance().FindOrCreateRenderPass(Span<TextureFormat>::FromVector(formats));
		}

		FLARE_CORE_ASSERT(m_Specifications.Width > 0 && m_Specifications.Height > 0);

		std::vector<VkImageView> imageViews;
		imageViews.reserve(m_Attachments.size());

		for (size_t i = 0; i < m_Specifications.Attachments.size(); i++)
		{
			if (IsDepthTextureFormat(m_Specifications.Attachments[i].Format))
			{
				m_DepthAttachmentIndex = (uint32_t)i;
				break;
			}
		}

		for (const auto& attachment : m_Attachments)
			imageViews.push_back(attachment->GetImageViewHandle());

		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.width = m_Specifications.Width;
		info.height = m_Specifications.Height;
		info.renderPass = m_CompatibleRenderPass->GetHandle();
		info.layers = 1;
		info.attachmentCount = (uint32_t)imageViews.size();
		info.pAttachments = imageViews.data();
		info.flags = 0;

		VK_CHECK_RESULT(vkCreateFramebuffer(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_FrameBuffer));

		if (m_OwnsAttachmentTextures)
		{
			FLARE_PROFILE_SCOPE("TransitionLayouts");
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
			for (size_t i = 0; i < m_Attachments.size(); i++)
			{
				if (IsDepthTextureFormat(m_Specifications.Attachments[i].Format))
				{
					commandBuffer->TransitionDepthImageLayout(m_Attachments[i]->GetImageHandle(),
						HasStencilComponent(m_Specifications.Attachments[i].Format),
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}
				else
				{
					commandBuffer->TransitionImageLayout(m_Attachments[i]->GetImageHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}
			}
			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
		}
	}

	void VulkanFrameBuffer::CreateImages()
	{
		m_Attachments.reserve(m_Specifications.Attachments.size());
		for (const FrameBufferAttachmentSpecifications& attachmentSpecifications : m_Specifications.Attachments)
		{
			TextureSpecifications specifications;
			specifications.Width = m_Specifications.Width;
			specifications.Height = m_Specifications.Height;
			specifications.Format = attachmentSpecifications.Format;
			specifications.Filtering = attachmentSpecifications.Filtering;
			specifications.Wrap = attachmentSpecifications.Wrap;
			specifications.GenerateMipMaps = false;
			specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;

			m_Attachments.push_back(As<VulkanTexture>(Texture::Create(specifications)));
		}
	}

	void VulkanFrameBuffer::ReleaseImages()
	{
		m_Attachments.clear();
	}
}
