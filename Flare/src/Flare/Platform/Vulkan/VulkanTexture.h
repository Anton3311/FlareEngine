#pragma once

#include "Flare/Renderer/Texture.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture();
		VulkanTexture(const std::filesystem::path& path, const TextureSpecifications& specifications);
		~VulkanTexture();

		void Bind(uint32_t slot) override;
		void SetData(const void* data, size_t size) override;
		const TextureSpecifications& GetSpecifications() const override;
		void* GetRendererId() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		TextureFormat GetFormat() const override;
		TextureFiltering GetFiltering() const override;

		inline VkImage GetImageHandle() const { return m_Image; }
		inline VkImageView GetImageViewHandle() const { return m_ImageView; }
		inline VkSampler GetDefaultSampler() const { return m_DefaultSampler; }
	private:
		TextureSpecifications m_Specifications;
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_DefaultSampler = VK_NULL_HANDLE;
	};
}
