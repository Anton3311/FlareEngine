#include "TextureImporter.h"

namespace Flare
{
	Ref<Texture> TextureImporter::ImportTexture(const AssetMetadata& metadata)
	{
		return Texture::Create(metadata.Path);
	}
}
