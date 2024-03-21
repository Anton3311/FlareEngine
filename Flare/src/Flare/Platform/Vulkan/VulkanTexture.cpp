#include "VulkanTexture.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <stb_image/stb_image.h>

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

		size_t imageSizeInBytes = 0;
		VkFormat imageFormat = VK_FORMAT_UNDEFINED;

		if (channels == 3)
		{
			m_Specifications.Format = TextureFormat::RGB8;
			imageSizeInBytes = width * height * 3;
			imageFormat = VK_FORMAT_R8G8B8_UNORM;
		}
		else if (channels == 4)
		{
			m_Specifications.Format = TextureFormat::RGBA8;
			imageSizeInBytes = width * height * 4;
			imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		}

		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};

		{
			VulkanContext::GetInstance().CreateBuffer(imageSizeInBytes,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

			void* mapped = nullptr;
			VK_CHECK_RESULT(vkMapMemory(VulkanContext::GetInstance().GetDevice(), stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &mapped));

			std::memcpy(mapped, data, imageSizeInBytes);

			vkUnmapMemory(VulkanContext::GetInstance().GetDevice(), stagingBufferMemory);
		}

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.arrayLayers = 1;
		imageInfo.extent.width = (uint32_t)width;
		imageInfo.extent.height = (uint32_t)height;
		imageInfo.extent.depth = 1;
		imageInfo.format = imageFormat;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.mipLevels = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VK_CHECK_RESULT(vkCreateImage(VulkanContext::GetInstance().GetDevice(), &imageInfo, nullptr, &m_Image));

		// Allocate memory
		VkMemoryRequirements memoryRequirements{};
		vkGetImageMemoryRequirements(VulkanContext::GetInstance().GetDevice(), m_Image, &memoryRequirements);

		uint32_t memoryType = VulkanContext::GetInstance().FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VkMemoryAllocateInfo allocation{};
		allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocation.allocationSize = memoryRequirements.size;
		allocation.memoryTypeIndex = memoryType;

		VK_CHECK_RESULT(vkAllocateMemory(VulkanContext::GetInstance().GetDevice(), &allocation, nullptr, &m_ImageMemory));
		VK_CHECK_RESULT(vkBindImageMemory(VulkanContext::GetInstance().GetDevice(), m_Image, m_ImageMemory, 0));

		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();

			commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkExtent3D size{};
			size.width = width;
			size.height = height;
			size.depth = 1;
			commandBuffer->CopyBufferToImage(stagingBuffer, m_Image, size);

			commandBuffer->TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
		}

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
		imageViewInfo.subresourceRange.levelCount = 1;
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

		vkFreeMemory(VulkanContext::GetInstance().GetDevice(), stagingBufferMemory, nullptr);
		vkDestroyBuffer(VulkanContext::GetInstance().GetDevice(), stagingBuffer, nullptr);
	}

	VulkanTexture::~VulkanTexture()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		VulkanContext::GetInstance().NotifyImageViewDeletionHandler(m_ImageView);

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		vkFreeMemory(device, m_ImageMemory, nullptr);
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

	void* VulkanTexture::GetRendererId() const
	{
		return nullptr;
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
}
