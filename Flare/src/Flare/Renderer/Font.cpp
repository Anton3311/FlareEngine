#include "Font.h"

#define MSDF_ATLAS_PUBLIC

#include <msdfgen.h>
#include <msdf-atlas-gen.h>
#include <msdfgen-ext.h>

namespace Flare
{
	Font::Font(const std::filesystem::path& path)
	{
        using namespace msdfgen;

        FreetypeHandle* ft = initializeFreetype();
        if (ft) {
            FontHandle* font = loadFont(ft, "C:\\Windows\\Fonts\\arialbd.ttf");
            if (font) {
                Shape shape;
                if (loadGlyph(shape, font, 'A')) {
                    shape.normalize();
                    //                      max. angle
                    edgeColoringSimple(shape, 3.0);
                    //           image width, height
                    Bitmap<float, 3> msdf(32, 32);
                    //                     range, scale, translation
                    generateMSDF(msdf, shape, 4.0, 1.0, Vector2(4.0, 4.0));
                    //savePng(msdf, "output.png");
                    saveBmp(msdf, "a.bmp");
                }
                destroyFont(font);
            }
            deinitializeFreetype(ft);
        }
	}
}