#include "OpenGLTexture.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>

#include <iostream>

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

			GLenum internalFormat = 0;
			GLenum dataFormat = 0;

			if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
				m_Format = TextureFormat::RGB8;
			}
			else if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
				m_Format = TextureFormat::RGBA8;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
			glBindTexture(GL_TEXTURE_2D, m_Id);
			glActiveTexture(GL_TEXTURE0);
			glTextureStorage2D(m_Id, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
	}
	
	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &m_Id);
	}

	void OpenGLTexture::Bind(uint32_t slot)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTextureUnit(slot, m_Id);
	}

	void OpenGLTexture::SetData(const void* data, size_t size)
	{
	}
}