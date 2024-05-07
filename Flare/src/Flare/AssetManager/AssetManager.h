#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/AssetManagerBase.h"

namespace Flare
{
	class FLARE_API AssetManager
	{
	public:
		static void Intialize(const Ref<AssetManagerBase>& assetManager);
		static void Uninitialize();

		static Ref<AssetManagerBase> GetInstance();
	public:
		template<typename T>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			return As<T>(GetRawAsset(handle));
		}

		static const AssetMetadata* GetAssetMetadata(AssetHandle handle);
		static Ref<Asset> GetRawAsset(AssetHandle handle);
		static bool IsAssetHandleValid(AssetHandle handle);
		static bool IsAssetLoaded(AssetHandle handle);
	};
}