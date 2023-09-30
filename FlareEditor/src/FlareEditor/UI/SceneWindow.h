#pragma once

#include "FlareEditor/UI/ECS/EntitiesHierarchy.h"

namespace Flare
{
	class SceneWindow
	{
	public:
		void OnImGuiRender();
	private:
		EntitiesHierarchy m_Hierarchy;
	};
}