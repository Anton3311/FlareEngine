#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/AssetManager.h"
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

		std::filesystem::path GetBuiltInContentPath() const;

		std::optional<AssetHandle> FindAssetByPath(const std::filesystem::path& path);

		AssetHandle ImportAsset(const std::filesystem::path& path, AssetHandle parentAsset = NULL_ASSET_HANDLE);
		AssetHandle ImportAsset(const std::filesystem::path& path, const Ref<Asset> asset, AssetHandle parentAsset = NULL_ASSET_HANDLE);
		AssetHandle ImportMemoryOnlyAsset(std::string_view name, const Ref<Asset> asset, AssetHandle parentAsset);

		void ReloadAsset(AssetHandle handle);
		void UnloadAsset(AssetHandle handle);

		void ReloadPrefabs();

		void RemoveFromRegistry(AssetHandle handle);
		void SetLoadedAsset(AssetHandle handle, const Ref<Asset>& asset);

		inline const EditorAssetRegistry& GetRegistry() const { return m_Registry; }

		Ref<Asset> LoadAsset(AssetHandle handle);

		inline static Ref<EditorAssetManager> GetInstance() { return As<EditorAssetManager>(AssetManager::GetInstance()); }

		void SerializeRegistry();

		static std::filesystem::path GetAssetsRoot();
	private:
		void RemoveFromRegistryWithoutSerialization(AssetHandle handle);

		void ImportBuiltinAssets();
		void ImportBuiltinAssets(const std::filesystem::path& directoryPath);

		Ref<Asset> LoadAsset(const AssetMetadata& metadata);
		void DeserializeRegistry();
	private:
		using AssetImporter = std::function<Ref<Asset>(const AssetMetadata&)>;

		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		EditorAssetRegistry m_Registry;

		std::unordered_map<std::filesystem::path, AssetHandle> m_FilepathToAssetHandle;
		std::unordered_map<AssetType, AssetImporter> m_AssetImporters;
	};
}