#pragma once

#include "Flare/Renderer/Texture.h"

namespace Flare
{
	class VulkanTexture : public Texture
	{
	public:
		void Bind(uint32_t slot) override;
		void SetData(const void* data, size_t size) override;
		const TextureSpecifications& GetSpecifications() const override;
		void* GetRendererId() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		TextureFormat GetFormat() const override;
		TextureFiltering GetFiltering() const override;
	private:
		TextureSpecifications m_Specifications;
	};
}
