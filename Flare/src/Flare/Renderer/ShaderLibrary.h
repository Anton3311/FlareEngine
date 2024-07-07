#pragma once

#include "Flare/AssetManager/Asset.h"

#include <string_view>
#include <optional>
#include <unordered_map>
#include <string>

namespace Flare
{
	class FLARE_API ShaderLibrary
	{
	public:
		static void AddShader(const std::string_view& name, AssetHandle handle);
		static void AddShader(AssetHandle handle);
		
		static void Remove(AssetHandle handle);
		static void Clear();

		static const std::unordered_map<std::string, AssetHandle>& GetNameToHandleMap();

		static std::optional<AssetHandle> FindShader(std::string_view name);
	};
}