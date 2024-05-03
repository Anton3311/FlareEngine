#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class ShaderImporter
	{
	public:
		static Ref<Asset> ImportShader(const AssetMetadata& metadata);
		static Ref<Asset> ImportComputeShader(const AssetMetadata& metadata);
	};
}