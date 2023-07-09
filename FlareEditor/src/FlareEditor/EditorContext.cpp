#include "EditorContext.h"

#include "Flare/AssetManager/AssetManager.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

namespace Flare
{
	EditorContext EditorContext::Instance{};

	void EditorContext::Initialize()
	{
		Instance.m_ActiveScene = CreateRef<Scene>();
		Instance.m_ActiveSceneHandle = NULL_ASSET_HANDLE;
	}

	void EditorContext::OpenScene(AssetHandle handle)
	{
		if (AssetManager::IsAssetHandleValid(handle))
		{
			Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());
			editorAssetManager->UnloadAsset(Instance.m_ActiveSceneHandle);
			
			Instance.m_ActiveSceneHandle = handle;
			Instance.m_ActiveScene = AssetManager::GetAsset<Scene>(handle);
		}
	}
}