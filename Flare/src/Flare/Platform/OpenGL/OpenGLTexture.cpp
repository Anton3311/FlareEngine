#include "OpenGLTexture.h"

#include "Flare.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>

namespace Flare
{
	OpenGLTexture::OpenGLTexture(const std::filesystem::path& path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

		if (data)
		{
			m_Width = width;
			m_Height = height;

			m_InternalTextureFormat = 0;
			m_TextureDataType = 0;

			if (channels == 3)
			{
				m_InternalTextureFormat = GL_RGB8;
				m_TextureDataType = GL_RGB;
				m_Format = TextureFormat::RGB8;
			}
			else if (channels == 4)
			{
				m_InternalTextureFormat = GL_RGBA8;
				m_TextureDataType = GL_RGBA;
				m_Format = TextureFormat::RGBA8;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
			glTextureStorage2D(m_Id, 1, m_InternalTextureFormat, m_Width, m_Height);

			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, m_TextureDataType, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
			FLARE_CORE_ERROR("Could not load an image {0}", path.string());
	}

	OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB8:
			m_InternalTextureFormat = GL_RGB8;
			m_TextureDataType = GL_RGB;
			break;
		case TextureFormat::RGBA8:
			m_InternalTextureFormat = GL_RGBA8;
			m_TextureDataType = GL_RGBA;
			break;
		}

		m_Width = width;
		m_Height = height;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
		glTextureStorage2D(m_Id, 1, m_InternalTextureFormat, m_Width, m_Height);

		glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, m_TextureDataType, GL_UNSIGNED_BYTE, data);
	}
	
	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &m_Id);
	}

	void OpenGLTexture::Bind(uint32_t slot)
	{
		glBindTextureUnit(slot, m_Id);
	}

	void OpenGLTexture::SetData(const void* data, size_t size)
	{
		glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, m_TextureDataType, GL_UNSIGNED_BYTE, data);
	}
}