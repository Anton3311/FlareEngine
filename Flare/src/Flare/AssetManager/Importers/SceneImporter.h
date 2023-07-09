#pragma once

#include "Flare/Scene/Scene.h"
#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class SceneImporter
	{
	public:
		static Ref<Scene> ImportScene(const AssetMetadata& metadata);
	};
}