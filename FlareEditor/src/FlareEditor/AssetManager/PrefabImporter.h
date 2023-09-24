#pragma once

#include "Flare/AssetManager/AssetManager.h"

namespace Flare
{
	class PrefabImporter
	{
	public:
		static Ref<Asset> ImportPrefab(const AssetMetadata& metadata);
	};
}