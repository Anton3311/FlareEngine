#pragma once

#include "Flare/Core/Core.h"

#include "Flare/AssetManager/Asset.h"

#include <unordered_map>

namespace Flare
{
	class AssetRegistry
	{
	private:
		std::unordered_map<AssetHandle, Ref<Asset>> m_LaodedAssets;
	};
}