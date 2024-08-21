#pragma once

#include "FlareCore/Core.h"

#include <filesystem>

namespace Flare
{
	struct AssetMetadata;
	class Mesh;
	class Prefab;
	class MeshImporter
	{
	public:
		static Ref<Mesh> ImportMesh(const AssetMetadata& metadata);
		static Ref<Prefab> ImportAsPrefab(const AssetMetadata& metadata);
	};
}