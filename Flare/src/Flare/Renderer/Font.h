#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/Texture.h"

#include <filesystem>
#include <vector>

namespace Flare
{
	struct MSDFData;

	class FLARE_API Font
	{
	public:
		Font(const std::filesystem::path& path);
		~Font();

		Ref<Texture> GetAtlas() const { return m_FontAtlas; }
	private:
		MSDFData* m_Data = nullptr;
		Ref<Texture> m_FontAtlas;
	};
}