#pragma once

#include "FlareCore/Core.h"

#include "Flare/AssetManager/Asset.h"

#include <filesystem>

namespace Flare
{
	class Mesh;
	class MeshSource;
	class Prefab;
	class MeshImporter
	{
	public:
		static Ref<Mesh> ImportMesh(const AssetMetadata& metadata);
		static Ref<Prefab> ImportAsPrefab(const AssetMetadata& metadata);

		static Ref<MeshSource> ImportMeshSource(const AssetMetadata& metadata);

		static void SerializeMesh(AssetHandle meshHandle, AssetHandle meshSourceHandle);
		static void SerializeMesh(const std::filesystem::path& path, AssetHandle meshSourceHandle);
	private:
		static bool DeserializeMeshSource(const std::filesystem::path& path, AssetHandle& outMeshSourceHandle);
	};
}