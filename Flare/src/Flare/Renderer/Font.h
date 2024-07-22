#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include "Flare/AssetManager/Asset.h"

#define MSDF_ATLAS_PUBLIC
#include <msdf-atlas-gen.h>

#include <filesystem>
#include <vector>

namespace Flare
{
	class Texture;

	struct MSDFData
	{
		MSDFData() = default;

		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
		msdf_atlas::FontGeometry Geometry;
	};

	class FLARE_API Font : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		Font() = default;
		Font(const std::filesystem::path& path);

		inline Ref<Texture> GetAtlas() const { return m_FontAtlas; }
		inline const MSDFData& GetData() const { return m_Data; }

		static Ref<Font> GetDefault();
		static void SetDefault(const Ref<Font>& font);
	private:
		MSDFData m_Data;
		Ref<Texture> m_FontAtlas = nullptr;
	};
}