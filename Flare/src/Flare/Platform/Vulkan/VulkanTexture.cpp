#include "VulkanTexture.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <stb_image/stb_image.h>
#include <glm/gtc/integer.hpp>

namespace Flare
{
	uint32_t CalculateMipCount(uint32_t width, uint32_t height)
	{
		return (uint32_t)glm::floor(glm::log2((float)glm::max(width, height))) + 1u;
	}

	VkFormat TextureFormatToVulkanFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::R11G11B10:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case TextureFormat::R32G32B32:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case TextureFormat::R32G32B32A32:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case TextureFormat::RG8:
			return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::RG16:
			return VK_FORMAT_R16G16_UNORM;
		case TextureFormat::RF32:
			return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::R8:
			return VK_FORMAT_R8_UNORM;

		case TextureFormat::Depth24Stencil8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::Depth32:
			return VK_FORMAT_D32_SFLOAT;

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



	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications, VkImage image, VkImageView imageView)
		: m_Specifications(specifications), m_Image(image), m_ImageView(imageView), m_OwnsImages(false)
	{
		FLARE_PROFILE_FUNCTION();
		CreateSampler();
	}

	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications)
		: m_Specifications(specifications)
	{
		FLARE_PROFILE_FUNCTION();
		CreateResources();
	}

	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications, MemorySpan pixelData)
		: m_Specifications(specifications)
	{
		FLARE_PROFILE_FUNCTION();
		CreateResources();
		UploadPixelData(Span<const MemorySpan>(&pixelData, 1));
	}

	VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
	{
		FLARE_PROFILE_FUNCTION();
		m_Specifications.Width = width;
		m_Specifications.Height = height;
		m_Specifications.Filtering = filtering;
		m_Specifications.Format = format;

		if (m_Specifications.GenerateMipMaps)
		{
			m_MipLevels = CalculateMipCount(m_Specifications.Width, m_Specifications.Height);
		}

		MemorySpan mipData = MemorySpan::FromRawBytes(data, m_Specifications.Width * m_Specifications.Height * GetImagePixelSizeInBytes());

		CreateResources();
		UploadPixelData(Span<const MemorySpan>(&mipData, 1));
	}

	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications, const void* data)
		: m_Specifications(specifications)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Specifications.GenerateMipMaps)
		{
			m_MipLevels = CalculateMipCount(m_Specifications.Width, m_Specifications.Height);
		}

		MemorySpan mipData = MemorySpan::FromRawBytes(data, m_Specifications.Width * m_Specifications.Height * GetImagePixelSizeInBytes());

		CreateResources();
		UploadPixelData(Span<const MemorySpan>(&mipData, 1));
	}

	VulkanTexture::VulkanTexture(const TextureSpecifications& specifications, const TexturePixelData& data)
		: m_Specifications(specifications)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(m_Specifications.GenerateMipMaps && data.Mips.size() == 1 || !m_Specifications.GenerateMipMaps);

		if (m_Specifications.GenerateMipMaps)
			m_MipLevels = CalculateMipCount(m_Specifications.Width, m_Specifications.Height);
		else
			m_MipLevels = (uint32_t)data.Mips.size();

		CreateResources();
		UploadPixelData(Span(data.Mips.data(), data.Mips.size()));
	}

	VulkanTexture::~VulkanTexture()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());

		ReleaseImage();
		vkDestroySampler(VulkanContext::GetInstance().GetDevice(), m_DefaultSampler, nullptr);

		m_DefaultSampler = VK_NULL_HANDLE;
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

	void VulkanTexture::Resize(uint32_t width, uint32_t height)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(m_OwnsImages);
		FLARE_CORE_ASSERT(width > 0 && height > 0);

		ReleaseImage();

		m_Specifications.Width = width;
		m_Specifications.Height = height;

		CreateImage();
		UpdateDebugName();
	}

	void VulkanTexture::SetDebugName(std::string_view debugName)
	{
		m_DebugName = debugName;

		UpdateDebugName();
	}

	const std::string& VulkanTexture::GetDebugName() const
	{
		return m_DebugName;
	}

	void VulkanTexture::CreateResources()
	{
		CreateImage();
		CreateSampler();
	}

	void VulkanTexture::CreateImage()
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
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.mipLevels = m_MipLevels;

		// TODO: Add TextureUsage::Blit?
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if (m_MipLevels > 1)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (HAS_BIT(m_Specifications.Usage, TextureUsage::Sampling))
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (HAS_BIT(m_Specifications.Usage, TextureUsage::RenderTarget))
		{
			if (IsDepthTextureFormat(m_Specifications.Format))
				imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.image = m_Image;

		{
			bool isDepth = IsDepthTextureFormat(m_Specifications.Format);
			bool hasStencil = HasStencilComponent(m_Specifications.Format);

			if (isDepth)
				imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			else
				imageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;

			// TODO: Do something with stencil aspect
#if 0
			if (hasStencil)
				imageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
#endif
		}

		VK_CHECK_RESULT(vkCreateImageView(VulkanContext::GetInstance().GetDevice(), &imageViewInfo, nullptr, &m_ImageView));
	}

	void VulkanTexture::CreateSampler()
	{
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

		if (m_MipLevels > 1)
		{
			samplerInfo.maxLod = (float)(m_MipLevels - 1);
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}

		VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetInstance().GetDevice(), &samplerInfo, nullptr, &m_DefaultSampler));
	}

	void VulkanTexture::UploadPixelData(Span<const MemorySpan> mips)
	{
		FLARE_CORE_ASSERT(mips.GetSize() > 0);
		VkFormat format = TextureFormatToVulkanFormat(m_Specifications.Format);

		size_t imageSize = 0;
		if (IsCompressedTextureFormat(m_Specifications.Format))
		{
			for (const auto& mip : mips)
			{
				FLARE_CORE_ASSERT(mip.GetSize() > 0);
				imageSize += mip.GetSize();
			}
		}
		else
		{
			uint32_t width = m_Specifications.Width;
			uint32_t height = m_Specifications.Height;
			size_t pixelSize = GetImagePixelSizeInBytes();

			for (const auto& mip : mips)
			{
				FLARE_CORE_ASSERT(mip.GetSize() > 0);
				imageSize += width * height * pixelSize;

				width /= 2;
				height /= 2;
			}
		}

		FLARE_CORE_ASSERT(imageSize > 0);

		VulkanStagingBufferPool& stagingBufferPool = VulkanContext::GetInstance().GetStagingBufferPool();
		VulkanStagingBuffer stagingBuffer;

		if (IsCompressedTextureFormat(m_Specifications.Format))
		{
			stagingBuffer = stagingBufferPool.AllocateStagingBuffer(imageSize);
		}
		else
		{
			size_t pixelSize = GetImagePixelSizeInBytes();
			stagingBuffer = stagingBufferPool.AllocateAlignedStagingBuffer(imageSize, pixelSize);
		}

		if (m_Specifications.Format == TextureFormat::RGB8)
		{
			FLARE_CORE_ASSERT(mips.GetSize() == 1);

			// Add alpha channel
			uint8_t* rgbaData = new uint8_t[imageSize];
			const uint8_t* oldData = (const uint8_t*)mips[0].GetBuffer();

			size_t j = 0;
			for (size_t i = 0; i < imageSize; i += 4)
			{
				rgbaData[i + 0] = oldData[j + 0];
				rgbaData[i + 1] = oldData[j + 1];
				rgbaData[i + 2] = oldData[j + 2];
				rgbaData[i + 3] = 255;

				j += 3;
			}

			std::memcpy(stagingBuffer.Mapped, rgbaData, imageSize);

			delete[] rgbaData;
		}
		else
		{
			size_t offset = 0;

			for (const auto& mip : mips)
			{
				std::memcpy((uint8_t*)stagingBuffer.Mapped + offset, mip.GetBuffer(), mip.GetSize());
				offset += mip.GetSize();
			}
		}

		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();

			uint32_t providedMipCount = (uint32_t)mips.GetSize();

			commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, providedMipCount);

			if (IsCompressedTextureFormat(m_Specifications.Format))
			{
				VkExtent3D size{};
				size.width = m_Specifications.Width;
				size.height = m_Specifications.Height;
				size.depth = 1;

				size_t bufferOffset = stagingBuffer.Offset;
				for (uint32_t i = 0; i < providedMipCount; i++)
				{
					commandBuffer->CopyBufferToImage(stagingBuffer.Buffer, m_Image, size, bufferOffset, i);

					bufferOffset += mips[i].GetSize();

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

				size_t bufferOffset = stagingBuffer.Offset;
				size_t pixelSize = GetImagePixelSizeInBytes();
				for (uint32_t i = 0; i < providedMipCount; i++)
				{
					commandBuffer->CopyBufferToImage(stagingBuffer.Buffer, m_Image, size, bufferOffset, i);

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

		stagingBufferPool.ReleaseStagingBuffer(stagingBuffer);
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

	void VulkanTexture::ReleaseImage()
	{
		VkDevice device = VulkanContext::GetInstance().GetDevice();
		if (m_OwnsImages)
		{
			VulkanContext::GetInstance().NotifyImageViewDeletionHandler(m_ImageView);
			vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle);

			vkDestroyImageView(device, m_ImageView, nullptr);
			vkDestroyImage(device, m_Image, nullptr);
		}
	}

	void VulkanTexture::UpdateDebugName()
	{
		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Image, m_DebugName.c_str());
		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_ImageView, m_DebugName.c_str());
	}
}
