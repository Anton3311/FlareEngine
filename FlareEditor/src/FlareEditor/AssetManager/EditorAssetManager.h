#pragma once

#include "Flare/Core/Core.h"
#include "Flare/AssetManager/AssetManagerBase.h"

#include <filesystem>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Flare
{
	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager(const std::filesystem::path& root);
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) override;
		virtual bool IsAssetHandleValid(AssetHandle handle) override;
		virtual bool IsAssetLoaded(AssetHandle handle) override;

		const std::filesystem::path& GetRoot() const { return m_Root; }
		std::optional<AssetHandle> FindAssetByPath(const std::filesystem::path& path);

		bool ImportAsset(const std::filesystem::path& path);
	private:
		void SerializeRegistry();
		void DeserializeRegistry();
	private:
		using AssetImporter = std::function<Ref<Asset>(const AssetMetadata&)>;

		std::filesystem::path m_Root;

		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		std::unordered_map<AssetHandle, AssetMetadata> m_Registry;

		std::unordered_map<std::filesystem::path, AssetHandle> m_FilepathToAssetHandle;
		std::unordered_map<AssetType, AssetImporter> m_AssetImporters;
	};
}