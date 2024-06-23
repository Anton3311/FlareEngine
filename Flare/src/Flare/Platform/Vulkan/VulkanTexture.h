#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	VkFormat TextureFormatToVulkanFormat(TextureFormat format);

	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(const TextureSpecifications& specifications, VkImage image, VkImageView imageView);
		VulkanTexture(const TextureSpecifications& specifications);
		VulkanTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering);
		VulkanTexture(const TextureSpecifications& specifications, const void* data);
		VulkanTexture(const TextureSpecifications& specifications, const TextureData& data);
		~VulkanTexture();

		void SetData(const void* data, size_t size) override;
		const TextureSpecifications& GetSpecifications() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		TextureFormat GetFormat() const override;
		TextureFiltering GetFiltering() const override;

		void Resize(uint32_t width, uint32_t height) override;

		void SetDebugName(std::string_view debugName) override;
		const std::string& GetDebugName() const override;

		inline VkImage GetImageHandle() const { return m_Image; }
		inline VkImageView GetImageViewHandle() const { return m_ImageView; }
		inline VkSampler GetDefaultSampler() const { return m_DefaultSampler; }
	private:
		void CreateResources();
		void CreateImage();
		void CreateSampler();
		void UploadPixelData(const TextureData& data);
		size_t GetImagePixelSizeInBytes();

		void ReleaseImage();
		void UpdateDebugName();
	private:
		std::string m_DebugName;

		TextureSpecifications m_Specifications;
		VulkanAllocation m_Allocation;

		uint32_t m_MipLevels = 1;

		bool m_OwnsImages = true;
		
		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_DefaultSampler = VK_NULL_HANDLE;
	};
}
