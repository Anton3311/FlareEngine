#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Project/Project.h"

#include <stdint.h>
#include <map>
#include <optional>

namespace Flare
{
	struct AssetRegistryEntry
	{
		bool IsBuiltIn = false;
		AssetMetadata Metadata;
	};

	class EditorAssetRegistry
	{
	public:
		inline const std::map<AssetHandle, AssetRegistryEntry>& GetEntries() const { return m_Entries; }
		inline bool Contains(AssetHandle handle) const { return m_Entries.find(handle) != m_Entries.end(); }

		inline AssetRegistryEntry* FindEntry(AssetHandle handle)
		{
			auto it = m_Entries.find(handle);
			if (it == m_Entries.end())
				return nullptr;

			return &it->second;
		}

		inline const AssetRegistryEntry* FindEntry(AssetHandle handle) const
		{
			auto it = m_Entries.find(handle);
			if (it == m_Entries.end())
				return nullptr;

			return &it->second;
		}

		inline AssetRegistryEntry& Insert(AssetHandle handle)
		{
			m_IsDirty = true;
			return m_Entries.emplace(handle, AssetRegistryEntry{}).first->second;
		}

		void Clear();

		bool Remove(AssetHandle handle);

		inline bool IsDirty() const { return m_IsDirty; }

		void Serialize(const std::filesystem::path& path);
		bool Deserialize(const std::filesystem::path& path);

		static const std::filesystem::path RegistryFileName;
		static const std::filesystem::path AssetsDirectoryName;
	private:
		bool m_IsDirty = false;
		std::map<AssetHandle, AssetRegistryEntry> m_Entries;
	};
}