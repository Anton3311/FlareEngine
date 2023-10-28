#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/AssetManagerBase.h"

#include <filesystem>
#include <map>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Flare
{
	constexpr char* ASSET_PAYLOAD_NAME = "ASSET_PAYLOAD";

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

		const std::filesystem::path& GetRoot() const { return m_Root; }
		std::optional<AssetHandle> FindAssetByPath(const std::filesystem::path& path);

		AssetHandle ImportAsset(const std::filesystem::path& path);
		AssetHandle ImportAsset(const std::filesystem::path& path, const Ref<Asset> asset);

		void ReloadAsset(AssetHandle handle);
		void UnloadAsset(AssetHandle handle);

		void ReloadPrefabs();

		void RemoveFromRegistry(AssetHandle handle);

		inline const std::map<AssetHandle, AssetMetadata>& GetRegistry() const { return m_Registry; }
	private:
		Ref<Asset> LoadAsset(const AssetMetadata& metadata);

		void SerializeRegistry();
		void DeserializeRegistry();
	private:
		using AssetImporter = std::function<Ref<Asset>(const AssetMetadata&)>;

		std::filesystem::path m_Root;

		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		std::map<AssetHandle, AssetMetadata> m_Registry;

		std::unordered_map<std::filesystem::path, AssetHandle> m_FilepathToAssetHandle;
		std::unordered_map<AssetType, AssetImporter> m_AssetImporters;

		static std::filesystem::path s_RegistryFileName;
		static std::filesystem::path s_AssetsDirectoryName;
	};
}