#pragma once

#include <Flare/Renderer/Texture.h>

namespace Flare
{
	class OpenGLTexture : public Texture
	{
	public:
		OpenGLTexture(const std::filesystem::path& path);
		~OpenGLTexture();
	public:
		virtual void Bind(uint32_t slot = 0) override;
		virtual void SetData(const void* data, size_t size) override;
	private:
		uint32_t m_Id;
	};
}