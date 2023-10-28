#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/AssetManagerBase.h"

#include "FlareEditor/AssetManager/EditorAssetRegistry.h"

#include <filesystem>
#include <map>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Flare
{
	constexpr char* ASSET_PAYLOAD_NAME = "ASSET_PAYLOAD";

	struct PackageEntry
	{
		UUID Id;
		std::string Name;
		std::filesystem::path Path;
	};

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager();

		void Reinitialize();
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) override;
		virtual const AssetMetadata* GetAssetMetadata(AssetHandle handle) override;

		virtual bool IsAssetHandleValid(AssetHandle handle) override;
		virtual bool IsAssetLoaded(AssetHandle handle) override;

		std::optional<AssetHandle> FindAssetByPath(const std::filesystem::path& path);

		AssetHandle ImportAsset(const std::filesystem::path& path);
		AssetHandle ImportAsset(const std::filesystem::path& path, const Ref<Asset> asset);

		void ReloadAsset(AssetHandle handle);
		void UnloadAsset(AssetHandle handle);

		void ReloadPrefabs();

		void RemoveFromRegistry(AssetHandle handle);

		inline const EditorAssetRegistry& GetRegistry() const { return m_Registry; }
	private:
		Ref<Asset> LoadAsset(const AssetMetadata& metadata);

		void SerializeRegistry();
		void DeserializeRegistry();
	private:
		using AssetImporter = std::function<Ref<Asset>(const AssetMetadata&)>;

		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		EditorAssetRegistry m_Registry;

		std::unordered_map<std::filesystem::path, AssetHandle> m_FilepathToAssetHandle;
		std::unordered_map<AssetType, AssetImporter> m_AssetImporters;

		std::map<UUID, PackageEntry> m_AssetPackages;
	};
}