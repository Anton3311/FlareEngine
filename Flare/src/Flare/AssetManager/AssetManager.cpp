#include "AssetManager.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	Ref<AssetManagerBase> s_Instance = nullptr;

	void AssetManager::Intialize(const Ref<AssetManagerBase>& assetManager)
	{
		s_Instance = assetManager;
	}

	void AssetManager::Uninitialize()
	{
		s_Instance = nullptr;
	}

	Ref<AssetManagerBase> AssetManager::GetInstance()
	{
		return s_Instance;
	}

	const AssetMetadata* AssetManager::GetAssetMetadata(AssetHandle handle)
	{
		FLARE_PROFILE_FUNCTION();
		return s_Instance->GetAssetMetadata(handle);
	}

	Ref<Asset> AssetManager::GetRawAsset(AssetHandle handle)
	{
		FLARE_PROFILE_FUNCTION();
		return s_Instance->GetAsset(handle);
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle handle)
	{
		FLARE_PROFILE_FUNCTION();
		return s_Instance->IsAssetHandleValid(handle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle handle)
	{
		FLARE_PROFILE_FUNCTION();
		return s_Instance->IsAssetLoaded(handle);
	}
}
