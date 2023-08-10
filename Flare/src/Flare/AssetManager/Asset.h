#pragma once

#include "Flare/Core/UUID.h"
#include "Flare/Core/Assert.h"

#include <filesystem>
#include <string_view>

namespace Flare
{
	using AssetHandle = UUID;
	constexpr AssetHandle NULL_ASSET_HANDLE = 0;

	enum class AssetType
	{
		None,
		Scene,
		Texture,
	};

	FLARE_API std::string_view AssetTypeToString(AssetType type);
	FLARE_API AssetType AssetTypeFromString(std::string_view string);

	struct AssetMetadata
	{
		AssetType Type;
		AssetHandle Handle;
		std::filesystem::path Path;
	};

	class FLARE_API Asset
	{
	public:
		Asset(AssetType type)
			: m_Type(type) {}
	public:
		inline AssetType GetType() const { return m_Type; }
	private:
		AssetType m_Type;
	public:
		AssetHandle Handle = NULL_ASSET_HANDLE;
	};
}