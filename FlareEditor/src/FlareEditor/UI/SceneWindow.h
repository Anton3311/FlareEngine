#pragma once

#include "FlareEditor/UI/ECS/EntitiesHierarchy.h"

namespace Flare
{
	class EditorCamera;
	class Scene;
	class SceneWindow
	{
	public:
		SceneWindow();
		void OnImGuiRender();

		void Initialize(Ref<Scene> scene, const EditorCamera* editorCamera);
		void Reset();
	private:
		EntitiesHierarchy m_Hierarchy;
		Ref<Scene> m_Scene = nullptr;
	};
}
