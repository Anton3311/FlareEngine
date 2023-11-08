#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	class ShaderImporter
	{
	public:
		static Ref<Asset> ImportShader(const AssetMetadata& metadata);
	};
}