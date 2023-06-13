#pragma once

#include "Flare/Core/Core.h"

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

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual TextureFormat GetFormat() const = 0;
	public:
		static Ref<Texture> Create(const std::filesystem::path& path);
		static Ref<Texture> Create(uint32_t width, uint32_t height, const void* data, TextureFormat format);
	};
}
