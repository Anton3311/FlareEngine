#pragma once

#include "Flare/AssetManager/Asset.h"

#include <filesystem>

namespace Flare
{
	struct MeshImportSettings
	{
		bool ImportMaterials = true;

		AssetHandle DefaultMaterial = NULL_ASSET_HANDLE;
	};

	class MeshImportSettingsSerializer
	{
	public:
		static std::filesystem::path GetImportSettingsPath(AssetHandle handle);

		static void Serialize(AssetHandle handle, const MeshImportSettings& settings);
		static bool Deserialize(AssetHandle handle, MeshImportSettings& outSettings);
	};
}
