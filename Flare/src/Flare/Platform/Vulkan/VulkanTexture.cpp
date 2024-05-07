#include "VulkanTexture.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <stb_image/stb_image.h>
#include <glm/gtc/integer.hpp>

namespace Flare
{
	VkFormat TextureFormatToVulkanFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::R32G32B32:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case TextureFormat::RG8:
			return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::RG16:
			return VK_FORMAT_R16G16_UNORM;
		case TextureFormat::RF32:
			return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::R8:
			return VK_FORMAT_R8_UNORM;

		case TextureFormat::BC1_RGB:
			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case TextureFormat::BC1_RGBA:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

		case TextureFormat::BC2_RGB:
			return VK_FORMAT_BC2_UNORM_BLOCK;
		case TextureFormat::BC3_RGB:
			return VK_FORMAT_BC3_UNORM_BLOCK;
		case TextureFormat::BC4_RGB:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case TextureFormat::BC5_RGB:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		}

		FLARE_CORE_ASSERT(false);
		return VK_FORMAT_UNDEFINED;
	}



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

		TextureData textureData{};
		auto& mip = textureData.Mips.emplace_back();
		mip.Data = data;
		mip.SizeInBytes = m_Specifications.Width * m_Specifications.Height * GetImagePixelSizeInBytes();

		if (m_Specifications.GenerateMipMaps)
		{
			m_MipLevels = (uint32_t)glm::floor(glm::log2((float)glm::max(m_Specifications.Width, m_Specifications.Height))) + 1u;
		}

		CreateResources();
		UploadPixelData(textureData);

		stbi_image_free(data);
	}

	VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
	{
		m_Specifications.Width = width;
		m_Specifications.Height = height;
		m_Specifications.Filtering = filtering;
		m_Specifications.Format = format;

		TextureData textureData{};
		auto& mip = textureData.Mips.emplace_back();
		mip.Data = data;
		mip.SizeInBytes = m_Specifications.Width * m_Specifications.Height * GetImagePixelSizeInBytes();

		if (m_Specifications.GenerateMipMaps)
		{
			m_MipLevels = (uint32_t)glm::floor(glm::log2((float)glm::max(m_Specifications.Width, m_Specifications.Height))) + 1u;
		}

		CreateResources();
		UploadPixelData(textureData);
	}

	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications, const void* data)
		: m_Specifications(specifications)
	{
		TextureData textureData{};
		auto& mip = textureData.Mips.emplace_back();
		mip.Data = data;
		mip.SizeInBytes = m_Specifications.Width * m_Specifications.Height * GetImagePixelSizeInBytes();

		if (m_Specifications.GenerateMipMaps)
		{
			m_MipLevels = (uint32_t)glm::floor(glm::log2((float)glm::max(m_Specifications.Width, m_Specifications.Height))) + 1u;
		}

		CreateResources();
		UploadPixelData(textureData);
	}

	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications, const TextureData& data)
		: m_Specifications(specifications)
	{
		m_MipLevels = (uint32_t)data.Mips.size();

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
		VkFormat imageFormat = TextureFormatToVulkanFormat(m_Specifications.Format);
		FLARE_CORE_ASSERT(imageFormat != VK_FORMAT_UNDEFINED);
		FLARE_CORE_ASSERT(m_MipLevels > 0);
		FLARE_CORE_ASSERT(m_Specifications.Width > 0 && m_Specifications.Height);

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
		imageInfo.mipLevels = m_MipLevels;

		if (m_MipLevels > 1)
		{
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
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.mipLodBias = 0.0f;

		switch (m_Specifications.Filtering)
		{
		case TextureFiltering::Closest:
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		case TextureFiltering::Linear:
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		switch (m_Specifications.Filtering)
		{
		case TextureFiltering::Closest:
			samplerInfo.minFilter = VK_FILTER_NEAREST;
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			break;
		case TextureFiltering::Linear:
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		if (m_Specifications.GenerateMipMaps)
		{
			samplerInfo.maxLod = (float)(m_MipLevels - 1);
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}

		VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetInstance().GetDevice(), &samplerInfo, nullptr, &m_DefaultSampler));
	}

	void VulkanTexture::UploadPixelData(const TextureData& data)
	{
		FLARE_CORE_ASSERT(data.Mips.size() > 0);
		VkFormat format = TextureFormatToVulkanFormat(m_Specifications.Format);

		size_t imageSize = 0;
		if (IsCompressedTextureFormat(m_Specifications.Format))
		{
			for (const auto& mip : data.Mips)
			{
				FLARE_CORE_ASSERT(mip.SizeInBytes > 0);
				imageSize += mip.SizeInBytes;
			}
		}
		else
		{
			uint32_t width = m_Specifications.Width;
			uint32_t height = m_Specifications.Height;
			size_t pixelSize = GetImagePixelSizeInBytes();

			for (const auto& mip : data.Mips)
			{
				FLARE_CORE_ASSERT(mip.SizeInBytes > 0);
				imageSize += width * height * pixelSize;

				width /= 2;
				height /= 2;
			}
		}

		FLARE_CORE_ASSERT(imageSize > 0);

		VkBuffer stagingBuffer{};
		VulkanAllocation stagingBufferAllocation = VulkanContext::GetInstance().CreateStagingBuffer(imageSize, stagingBuffer);

		void* mapped = nullptr;
		VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle, &mapped));

		if (m_Specifications.Format == TextureFormat::RGB8)
		{
			FLARE_CORE_ASSERT(data.Mips.size() == 1);

			// Add alpha channel
			uint8_t* rgbaData = new uint8_t[imageSize];
			const uint8_t* oldData = (const uint8_t*)data.Mips[0].Data;

			size_t j = 0;
			for (size_t i = 0; i < imageSize; i += 4)
			{
				rgbaData[i + 0] = oldData[j + 0];
				rgbaData[i + 1] = oldData[j + 1];
				rgbaData[i + 2] = oldData[j + 2];
				rgbaData[i + 3] = 255;

				j += 3;
			}

			std::memcpy(mapped, rgbaData, imageSize);

			delete[] rgbaData;
		}
		else
		{
			size_t offset = 0;

			for (const auto& mip : data.Mips)
			{
				std::memcpy((uint8_t*)mapped + offset, mip.Data, mip.SizeInBytes);
				offset += mip.SizeInBytes;
			}
		}

		VK_CHECK_RESULT(vmaFlushAllocation(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle, 0, VK_WHOLE_SIZE));
		vmaUnmapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle);

		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();

			uint32_t providedMipCount = (uint32_t)data.Mips.size();

			commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, providedMipCount);

			if (IsCompressedTextureFormat(m_Specifications.Format))
			{
				VkExtent3D size{};
				size.width = m_Specifications.Width;
				size.height = m_Specifications.Height;
				size.depth = 1;

				size_t bufferOffset = 0;
				for (uint32_t i = 0; i < providedMipCount; i++)
				{
					commandBuffer->CopyBufferToImage(stagingBuffer, m_Image, size, bufferOffset, i);

					bufferOffset += data.Mips[i].SizeInBytes;

					size.width /= 2;
					size.height /= 2;
				}
			}
			else
			{
				VkExtent3D size{};
				size.width = m_Specifications.Width;
				size.height = m_Specifications.Height;
				size.depth = 1;

				size_t bufferOffset = 0;
				size_t pixelSize = GetImagePixelSizeInBytes();
				for (uint32_t i = 0; i < providedMipCount; i++)
				{
					commandBuffer->CopyBufferToImage(stagingBuffer, m_Image, size, bufferOffset, i);

					bufferOffset += size.width * size.height * pixelSize;

					size.width /= 2;
					size.height /= 2;
				}
			}

			if (m_Specifications.GenerateMipMaps && m_MipLevels > 1 && providedMipCount == 1)
			{
				commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);
				commandBuffer->GenerateImageMipMaps(m_Image, m_MipLevels, glm::uvec2(m_Specifications.Width, m_Specifications.Height));
				commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, m_MipLevels);
			}
			else
			{
				commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, providedMipCount);
			}

			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
		}

		vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), stagingBufferAllocation.Handle);
		vkDestroyBuffer(VulkanContext::GetInstance().GetDevice(), stagingBuffer, nullptr);
	}

	size_t VulkanTexture::GetImagePixelSizeInBytes()
	{
		size_t pixelSize = 0;

		switch (m_Specifications.Format)
		{
		case TextureFormat::RGB8:
			pixelSize = 4;
			break;
		case TextureFormat::RGBA8:
			pixelSize = 4;
			break;
		case TextureFormat::RG8:
			pixelSize = 2;
			break;
		case TextureFormat::RG16:
			pixelSize = 4;
			break;
		case TextureFormat::RF32:
			pixelSize = sizeof(float);
			break;
		case TextureFormat::R8:
			pixelSize = 1;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		return pixelSize;
	}
}
