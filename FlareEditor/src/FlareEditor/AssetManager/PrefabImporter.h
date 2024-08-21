#pragma once

#include "Flare/AssetManager/AssetManager.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	class Prefab;
	class World;
	class PrefabImporter
	{
	public:
		static void SerializePrefab(AssetHandle prefab);
		static void SerializeGeneratedPrefab(Ref<Prefab> prefab, AssetHandle generatorHandle);
		static Ref<Asset> ImportPrefab(const AssetMetadata& metadata);
	};
}