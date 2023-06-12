#pragma once

#include <Flare/Core/Core.h>

#include <filesystem>

namespace Flare
{
	enum class TextureFormat
	{
		RGB8,
		RGBA8,
	};

	class Texture
	{
	public:
		virtual void Bind(uint32_t slot = 0) = 0;
		virtual void SetData(const void* data, size_t size) = 0;
	protected:
		TextureFormat m_Format;
		uint32_t m_Width;
		uint32_t m_Height;
	public:
		static Ref<Texture> Create(const std::filesystem::path& path);
		static Ref<Texture> Create(uint32_t width, uint32_t height, const void* data, TextureFormat format);
	};
}
