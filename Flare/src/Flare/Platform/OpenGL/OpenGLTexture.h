#pragma once

#include <Flare/Renderer/Texture.h>

#include <glad/glad.h>

namespace Flare
{
	class OpenGLTexture : public Texture
	{
	public:
		OpenGLTexture(const std::filesystem::path& path);
		OpenGLTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format);
		~OpenGLTexture();
	public:
		virtual void Bind(uint32_t slot = 0) override;
		virtual void SetData(const void* data, size_t size) override;
	private:
		uint32_t m_Id;
		GLenum m_InternalTextureFormat;
		GLenum m_TextureDataType;
	};
}