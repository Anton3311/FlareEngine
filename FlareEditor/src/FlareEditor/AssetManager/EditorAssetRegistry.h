#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Project/Project.h"

#include <stdint.h>
#include <map>
#include <optional>

namespace Flare
{
	enum class AssetOwner : uint8_t
	{
		Project,
		Package,
	};

	struct AssetRegistryEntry
	{
		AssetOwner OwnerType;
		std::optional<UUID> PackageId;
		AssetMetadata Metadata;
	};

	using EditorAssetRegistry = std::map<AssetHandle, AssetRegistryEntry>;

	class AssetRegistrySerializer
	{
	public:
		static void Serialize(const EditorAssetRegistry& registry, const std::filesystem::path& path);
		static bool Deserialize(EditorAssetRegistry& registry, const std::filesystem::path& path);

		inline static std::filesystem::path GetAssetsRoot()
		{
			return Project::GetActive()->Location / AssetsDirectoryName;
		}

		static const std::filesystem::path RegistryFileName;
		static const std::filesystem::path AssetsDirectoryName;
	};
}