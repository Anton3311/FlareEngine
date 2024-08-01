#pragma once

#include "Flare/AssetManager/AssetManager.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	class World;
	class PrefabImporter
	{
	public:
		static void SerializePrefab(AssetHandle prefab, World& world, Entity entity);
		static Ref<Asset> ImportPrefab(const AssetMetadata& metadata);
	};
}