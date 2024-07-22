#include "Font.h"

#include "FlareCore/Assert.h"

#include "Flare/Renderer/Texture.h"

#include <msdfgen.h>
#include <msdfgen-ext.h>

namespace Flare
{
    static Ref<Font> s_DefualtFont = nullptr;

    FLARE_SERIALIZABLE_IMPL(Font);
    FLARE_IMPL_ASSET(Font);

	Font::Font(const std::filesystem::path& path)
        : Asset(AssetType::Font)
	{
        msdfgen::FreetypeHandle* freetype = msdfgen::initializeFreetype();
        
        FLARE_CORE_ASSERT(freetype);
        msdfgen::FontHandle* fontHandle = msdfgen::loadFont(freetype, path.string().c_str());

        if (!fontHandle)
            FLARE_CORE_ERROR("Failed to load font: {0}", path.generic_string());
        else
        {
            struct CharsetRange
            {
                uint32_t Start;
                uint32_t End;
            };

            static const CharsetRange RANGES[] =
            {
                { 0x0020, 0x00ff },
            };

            msdf_atlas::Charset charset;
            
            for (size_t i = 0; i < sizeof(RANGES) / sizeof(RANGES[0]); i++)
            {
                for (uint32_t c = RANGES[i].Start; c <= RANGES[i].End; c++)
                    charset.add(c);
            }

            double fontScale = 1.0f;

            m_Data.Geometry = msdf_atlas::FontGeometry(&m_Data.Glyphs);
            m_Data.Geometry.loadCharset(fontHandle, fontScale, charset);

            double emSize = 40.0;

            msdf_atlas::TightAtlasPacker atlasPacker;
            atlasPacker.setPixelRange(2.0);
            atlasPacker.setMiterLimit(1.0);
            atlasPacker.setPadding(0);
            atlasPacker.setScale(emSize);

            constexpr uint64_t LCG_MULTIPLIER = 6364136223846793005ull;
            constexpr double DEFAULT_ANGLE_THRESHOLD = 3.0;

            uint64_t glyphSeed = 0;
            for (msdf_atlas::GlyphGeometry& glyph : m_Data.Glyphs)
            {
                glyphSeed *= LCG_MULTIPLIER;
                glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
            }

            int32_t remaining = atlasPacker.pack(m_Data.Glyphs.data(), (int32_t)m_Data.Glyphs.size());
            FLARE_CORE_ASSERT(remaining == 0);

            int32_t width;
            int32_t height;

            atlasPacker.getDimensions(width, height);

            msdf_atlas::GeneratorAttributes attributes;
            attributes.config.overlapSupport = true;
            attributes.scanlinePass = true;

            msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<uint8_t, 3>> generator(width, height);
            generator.setAttributes(attributes);
            generator.setThreadCount(8);
            generator.generate(m_Data.Glyphs.data(), (int)m_Data.Glyphs.size());

            msdfgen::BitmapConstRef<uint8_t, 3> bitmap = (msdfgen::BitmapConstRef<uint8_t, 3>)generator.atlasStorage();

            m_FontAtlas = Texture::Create(width, height, bitmap.pixels, TextureFormat::RGB8);
        }

        msdfgen::deinitializeFreetype(freetype);
	}

    Ref<Font> Font::GetDefault()
    {
        return s_DefualtFont;
    }

    void Font::SetDefault(const Ref<Font>& font)
    {
        s_DefualtFont = font;
    }
}