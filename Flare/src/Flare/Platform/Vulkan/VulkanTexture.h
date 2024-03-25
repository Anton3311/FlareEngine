#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture();
		VulkanTexture(const std::filesystem::path& path, const TextureSpecifications& specifications);
		VulkanTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering);
		~VulkanTexture();

		void Bind(uint32_t slot) override;
		void SetData(const void* data, size_t size) override;
		const TextureSpecifications& GetSpecifications() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		TextureFormat GetFormat() const override;
		TextureFiltering GetFiltering() const override;

		inline VkImage GetImageHandle() const { return m_Image; }
		inline VkImageView GetImageViewHandle() const { return m_ImageView; }
		inline VkSampler GetDefaultSampler() const { return m_DefaultSampler; }
	private:
		void CreateResources();
		void UploadPixelData(const void* data);
		void GetImageSizeAndFormat(size_t& size, VkFormat& format);
	private:
		TextureSpecifications m_Specifications;
		VulkanAllocation m_Allocation;
		
		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_DefaultSampler = VK_NULL_HANDLE;
	};
}
