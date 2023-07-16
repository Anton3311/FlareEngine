#include "EditorContext.h"

#include "Flare/AssetManager/AssetManager.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

namespace Flare
{
	EditorContext EditorContext::Instance{};

	void EditorContext::Initialize()
	{
		Instance.m_ActiveScene = CreateRef<Scene>();
		Instance.m_EditedScene = Instance.m_ActiveScene;
	}

	void EditorContext::OpenScene(AssetHandle handle)
	{
		FLARE_CORE_ASSERT(Instance.Mode == EditorMode::Edit);

		if (AssetManager::IsAssetHandleValid(handle))
		{
			Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

			if (AssetManager::IsAssetHandleValid(Instance.m_ActiveScene->Handle))
				editorAssetManager->UnloadAsset(Instance.m_ActiveScene->Handle);
			
			Instance.m_ActiveScene = AssetManager::GetAsset<Scene>(handle);
			Instance.m_EditedScene = Instance.m_ActiveScene;
		}
	}
}