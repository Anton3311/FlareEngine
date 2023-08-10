#pragma once

#include "Flare/Core/Core.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/AssetManager/Asset.h"

#include <filesystem>

namespace Flare
{
	class FLARE_API TextureImporter
	{
	public:
		static Ref<Texture> ImportTexture(const AssetMetadata& metadata);
	};
}