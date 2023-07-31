#include "OpenGLTexture.h"

#include "Flare.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>

namespace Flare
{
	OpenGLTexture::OpenGLTexture(const std::filesystem::path& path, TextureFiltering filtering)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

		if (data)
		{
			m_Specifications.Width = width;
			m_Specifications.Height = height;
			m_Specifications.Filtering = filtering;

			m_InternalTextureFormat = 0;
			m_TextureDataType = 0;

			if (channels == 3)
			{
				m_InternalTextureFormat = GL_RGB8;
				m_TextureDataType = GL_RGB;
				m_Specifications.Format = TextureFormat::RGB8;
			}
			else if (channels == 4)
			{
				m_InternalTextureFormat = GL_RGBA8;
				m_TextureDataType = GL_RGBA;
				m_Specifications.Format = TextureFormat::RGBA8;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
			glTextureStorage2D(m_Id, 1, m_InternalTextureFormat, m_Specifications.Width, m_Specifications.Height);

			SetFiltering(m_Specifications.Filtering);

			glTextureSubImage2D(m_Id, 0, 0, 0, m_Specifications.Width, m_Specifications.Height, m_TextureDataType, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
			FLARE_CORE_ERROR("Could not load an image {0}", path.string());
	}

	OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height, const void* data, TextureFormat format, TextureFiltering filtering)
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

		m_Specifications.Width = width;
		m_Specifications.Height = height;
		m_Specifications.Filtering = filtering;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
		glTextureStorage2D(m_Id, 1, m_InternalTextureFormat, m_Specifications.Width, m_Specifications.Height);

		SetFiltering(m_Specifications.Filtering);

		glTextureSubImage2D(m_Id, 0, 0, 0, m_Specifications.Width, m_Specifications.Height, m_TextureDataType, GL_UNSIGNED_BYTE, data);
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
		glTextureSubImage2D(m_Id, 0, 0, 0, m_Specifications.Width, m_Specifications.Height, m_TextureDataType, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture::SetFiltering(TextureFiltering filtering)
	{
		switch (m_Specifications.Filtering)
		{
		case TextureFiltering::Closest:
			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case TextureFiltering::Linear:
			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}
	}
}