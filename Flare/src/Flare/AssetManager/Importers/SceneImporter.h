#pragma once

#include "FlareCore/Core.h"

#include "Flare/Scene/Scene.h"
#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class FLARE_API SceneImporter
	{
	public:
		static Ref<Scene> ImportScene(const AssetMetadata& metadata);
	};
}