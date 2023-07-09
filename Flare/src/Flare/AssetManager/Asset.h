#pragma once

#include "Flare/Core/UUID.h"
#include "Flare/Core/Assert.h"

#include <filesystem>
#include <string_view>

namespace Flare
{
	using AssetHandle = UUID;

	enum class AssetType
	{
		None,
		Scene,
		Texture,
	};

	std::string_view AssetTypeToString(AssetType type);

	struct AssetMetadata
	{
		AssetType Type;
		AssetHandle Handle;
		std::filesystem::path Path;
	};

	class Asset
	{
	public:
		Asset(AssetType type)
			: m_Type(type) {}
	public:
		inline AssetType GetType() const { return m_Type; }
	private:
		AssetType m_Type;
	};
}