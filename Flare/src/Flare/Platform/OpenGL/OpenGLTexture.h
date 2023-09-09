#pragma once

#include "Flare/Renderer/Texture.h"

#include <glad/glad.h>

namespace Flare
{
	class OpenGLTexture : public Texture
	{
	public:
		// Texture size and format are ignored, because they are retrieved from the file
		OpenGLTexture(const std::filesystem::path& path, const TextureSpecifications& specifications);
		OpenGLTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering);
		~OpenGLTexture();
	public:
		virtual void Bind(uint32_t slot = 0) override;
		virtual void SetData(const void* data, size_t size) override;

		virtual const TextureSpecifications& GetSpecifications() const override { return m_Specifications; }

		virtual uint32_t GetWidth() const override { return m_Specifications.Width; }
		virtual uint32_t GetHeight() const override { return m_Specifications.Height; }
		virtual TextureFormat GetFormat() const override { return m_Specifications.Format; }
		virtual TextureFiltering GetFiltering() const override { return m_Specifications.Filtering; }
	private:
		void SetFiltering(TextureFiltering filtering);
	private:
		uint32_t m_Id;
		TextureSpecifications m_Specifications;

		GLenum m_InternalTextureFormat;
		GLenum m_TextureDataType;
	};
}