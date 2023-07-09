#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Scene/Scene.h"

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	struct EditorContext
	{
	public:
		static void Initialize();
		static void OpenScene(AssetHandle handle);

		static const Ref<Scene>& GetActiveScene() { return Instance.m_ActiveScene; }
		static AssetHandle GetActiveSceneHandle() { return Instance.m_ActiveSceneHandle; }
	public:

		Entity SelectedEntity;
	private:
		Ref<Scene> m_ActiveScene;
		AssetHandle m_ActiveSceneHandle;
	public:
		static EditorContext Instance;
	};
}