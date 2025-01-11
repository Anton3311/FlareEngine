#pragma once

#include "FlareEditor/UI/ECS/EntitiesHierarchy.h"

namespace Flare
{
	class Scene;
	class SceneWindow
	{
	public:
		SceneWindow();
		void OnImGuiRender();

		void SetScene(Ref<Scene> scene);
		void Reset();
	private:
		EntitiesHierarchy m_Hierarchy;
		Ref<Scene> m_Scene = nullptr;
	};
}