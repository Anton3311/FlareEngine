#pragma once

#include "Flare/Core/Core.h"
#include "Flare/AssetManager/AssetManagerBase.h"

namespace Flare
{
	class AssetManager
	{
	public:
		static void Intialize(const Ref<AssetManagerBase>& assetManager);

		static Ref<AssetManagerBase> GetInstance() { return s_Instance; }
	public:
		template<typename T>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of<Asset, T>, "Provided type is not an asset type");
			return As<T>(GetRawAsset(handle));
		}
		
		static Ref<Asset> GetRawAsset(AssetHandle handle)
		{
			return s_Instance->GetAsset(handle);
		}

		static bool IsAssetHandleValid(AssetHandle handle)
		{
			return s_Instance->IsAssetHandleValid(handle);
		}

		static bool IsAssetLoaded(AssetHandle handle)
		{
			return s_Instance->IsAssetLoaded(handle);
		}
	private:
		static Ref<AssetManagerBase> s_Instance;
	};
}