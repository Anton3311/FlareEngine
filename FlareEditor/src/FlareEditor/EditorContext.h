#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Scene/Scene.h"

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	enum class EditorMode
	{
		Edit,
		Play,
	};

	struct EditorContext
	{
	public:
		static void Initialize();
		static void OpenScene(AssetHandle handle);

		static const Ref<Scene>& GetActiveScene() { return Instance.m_ActiveScene; }
		static const Ref<Scene>& GetEditedScene() { return Instance.m_EditedScene; }

		static void SetActiveScene(const Ref<Scene>& scene) { Instance.m_ActiveScene = scene; }
	public:
		Entity SelectedEntity;
		EditorMode Mode;
	private:
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditedScene;
	public:
		static EditorContext Instance;
	};
}