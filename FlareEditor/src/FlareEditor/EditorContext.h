#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Scene/Scene.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	struct EditorContext
	{
	public:
		Ref<Scene> ActiveScene;
		Entity SelectedEntity;
		
		static EditorContext Instance;
	};
}