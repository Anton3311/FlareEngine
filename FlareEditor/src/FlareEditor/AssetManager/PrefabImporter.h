#pragma once

#include "Flare/AssetManager/AssetManager.h"

#include "FlareECS/World.h"

namespace Flare
{
	class PrefabImporter
	{
	public:
		static void SerializePrefab(AssetHandle prefab, World& world, Entity entity);
		static Ref<Asset> ImportPrefab(const AssetMetadata& metadata);
	};
}