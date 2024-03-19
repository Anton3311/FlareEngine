#include "VulkanTexture.h"

namespace Flare
{
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
		return 0;
	}

	uint32_t VulkanTexture::GetHeight() const
	{
		return 0;
	}

	TextureFormat VulkanTexture::GetFormat() const
	{
		return TextureFormat::RGBA8;
	}

	TextureFiltering VulkanTexture::GetFiltering() const
	{
		return TextureFiltering::Closest;
	}
}
