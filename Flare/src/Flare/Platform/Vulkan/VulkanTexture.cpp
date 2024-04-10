#include "VulkanTexture.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <stb_image/stb_image.h>
#include <glm/gtc/integer.hpp>

namespace Flare
{
	VulkanTexture::VulkanTexture()
	{
	}

	VulkanTexture::VulkanTexture(const std::filesystem::path& path, const TextureSpecifications& specifications)
		: m_Specifications(specifications)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

		if (!data)
		{
			FLARE_CORE_ERROR("Failed to load texture: {}", path.string());
			return;
		}

		m_Specifications = specifications;
		m_Specifications.Width = width;
		m_Specifications.Height = height;
		
		if (channels == 3)
			m_Specifications.Format = TextureFormat::RGB8;
		else if (channels == 4)
			m_Specifications.Format = TextureFormat::RGBA8;

		CreateResources();
		UploadPixelData(data);
	}

	VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
	{
		m_Specifications.Width = width;
		m_Specifications.Height = height;
		m_Specifications.Filtering = filtering;
		m_Specifications.Format = format;

		CreateResources();
		UploadPixelData(data);
	}

	VulkanTexture::~VulkanTexture()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		VulkanContext::GetInstance().NotifyImageViewDeletionHandler(m_ImageView);

		vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle);

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		vkDestroySampler(device, m_DefaultSampler, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkDestroyImage(device, m_Image, nullptr);
	}

	void VulkanTexture::Bind(uint32_t slot)
	{
	}

	void VulkanTexture::SetData(const void* data, size_t size)
	{
	}

	const TextureSpecifications& VulkanTexture::GetSpecifications() const
	{
		return m_Specifications;
	}

	uint32_t VulkanTexture::GetWidth() const
	{
		return m_Specifications.Width;
	}

	uint32_t VulkanTexture::GetHeight() const
	{
		return m_Specifications.Height;
	}

	TextureFormat VulkanTexture::GetFormat() const
	{
		return m_Specifications.Format;
	}

	TextureFiltering VulkanTexture::GetFiltering() const
	{
		return m_Specifications.Filtering;
	}

	void VulkanTexture::CreateResources()
	{
		VkFormat imageFormat = VK_FORMAT_UNDEFINED;
		size_t imageSize = 0;

		GetImageSizeAndFormat(imageSize, imageFormat);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.arrayLayers = 1;
		imageInfo.extent.width = (uint32_t)m_Specifications.Width;
		imageInfo.extent.height = (uint32_t)m_Specifications.Height;
		imageInfo.extent.depth = 1;
		imageInfo.format = imageFormat;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.mipLevels = 1;

		if (m_Specifications.GenerateMipMaps)
		{
			m_MipLevels = (uint32_t)glm::floor(glm::log2((float)glm::max(imageInfo.extent.width, imageInfo.extent.height))) + 1u;

			imageInfo.mipLevels = m_MipLevels;
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		VmaAllocationCreateInfo allocation{};
		allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		VK_CHECK_RESULT(vmaCreateImage(VulkanContext::GetInstance().GetMemoryAllocator(), &imageInfo, &allocation, &m_Image, &m_Allocation.Handle, &m_Allocation.Info));

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewInfo.flags = 0;
		imageViewInfo.format = imageFormat;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.image = m_Image;

		VK_CHECK_RESULT(vkCreateImageView(VulkanContext::GetInstance().GetDevice(), &imageViewInfo, nullptr, &m_ImageView));

		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		switch (m_Specifications.Wrap)
		{
		case TextureWrap::Clamp:
			addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case TextureWrap::Repeat:
			addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		}

		VkFilter filter = VK_FILTER_NEAREST;
		switch (m_Specifications.Filtering)
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

		VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetInstance().GetDevice(), &samplerInfo, nullptr, &m_DefaultSampler));
	}

	void VulkanTexture::UploadPixelData(const void* data)
	{
		VkFormat format = VK_FORMAT_UNDEFINED;
		size_t size = 0;
		GetImageSizeAndFormat(size, format);

		VkBuffer stagingBuffer{};
		VulkanAllocation stagingBufferAllocation = VulkanContext::GetInstance().CreateStagingBuffer(size, stagingBuffer);

		void* mapped = nullptr;
		VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle, &mapped));

		if (m_Specifications.Format == TextureFormat::RGB8)
		{
			// Add alpha channel
			uint8_t* rgbaData = new uint8_t[size];
			const uint8_t* oldData = (const uint8_t*)data;

			size_t j = 0;
			for (size_t i = 0; i < size; i += 4)
			{
				rgbaData[i + 0] = oldData[j + 0];
				rgbaData[i + 1] = oldData[j + 1];
				rgbaData[i + 2] = oldData[j + 2];
				rgbaData[i + 3] = 255;

				j += 3;
			}

			std::memcpy(mapped, rgbaData, size);

			delete[] rgbaData;
		}
		else
		{
			std::memcpy(mapped, data, size);
		}

		VK_CHECK_RESULT(vmaFlushAllocation(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle, 0, VK_WHOLE_SIZE));
		vmaUnmapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle);

		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();

			commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkExtent3D size{};
			size.width = m_Specifications.Width;
			size.height = m_Specifications.Height;
			size.depth = 1;
			commandBuffer->CopyBufferToImage(stagingBuffer, m_Image, size);

			if (m_Specifications.GenerateMipMaps && m_MipLevels > 1)
			{
				commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);
				commandBuffer->GenerateImageMipMaps(m_Image, m_MipLevels, glm::uvec2(m_Specifications.Width, m_Specifications.Height));
				commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, m_MipLevels);
			}
			else
			{
				commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
		}

		vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle);
		vkDestroyBuffer(VulkanContext::GetInstance().GetDevice(), stagingBuffer, nullptr);
	}

	void VulkanTexture::GetImageSizeAndFormat(size_t& size, VkFormat& format)
	{
		size_t pixelSize = 0;

		switch (m_Specifications.Format)
		{
		case TextureFormat::RGB8:
			format = VK_FORMAT_R8G8B8A8_UNORM;
			pixelSize = 4;
			break;
		case TextureFormat::RGBA8:
			format = VK_FORMAT_R8G8B8A8_UNORM;
			pixelSize = 4;
			break;
		case TextureFormat::RG8:
			format = VK_FORMAT_R8G8_UNORM;
			pixelSize = 2;
			break;
		case TextureFormat::RG16:
			format = VK_FORMAT_R16G16_UNORM;
			pixelSize = 4;
			break;
		case TextureFormat::RF32:
			format = VK_FORMAT_R16_SFLOAT;
			pixelSize = sizeof(float);
			break;
		case TextureFormat::R8:
			format = VK_FORMAT_R8_UNORM;
			pixelSize = 1;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		size = m_Specifications.Width * m_Specifications.Height * pixelSize;
	}
}
