#include "TextureImporter.h"

namespace Flare
{
	Ref<Texture> TextureImporter::ImportTexture(const AssetMetadata& metadata)
	{
		// NOTE: TextureFiltering::Closest is default, util asset import settings are implemented
		return Texture::Create(metadata.Path, TextureFiltering::Closest);
	}
}
