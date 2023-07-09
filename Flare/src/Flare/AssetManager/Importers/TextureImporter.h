#pragma once

#include "Flare/Renderer/Texture.h"

#include "Flare/AssetManager/Asset.h"

#include <filesystem>

namespace Flare
{
	class TextureImporter
	{
	public:
		static Ref<Texture> ImportTexture(const AssetMetadata& metadata);
	};
}