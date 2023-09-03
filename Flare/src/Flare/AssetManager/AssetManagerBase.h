#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/Asset.h"

#include <optional>

namespace Flare
{
	class AssetManagerBase
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) = 0;
		virtual const AssetMetadata* GetAssetMetadata(AssetHandle handle) = 0;

		virtual bool IsAssetHandleValid(AssetHandle handle) = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) = 0;
	};
}