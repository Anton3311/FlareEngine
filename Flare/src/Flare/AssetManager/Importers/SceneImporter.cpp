#include "SceneImporter.h"

#include "Flare/Scene/SceneSerializer.h"

namespace Flare
{
	Ref<Scene> SceneImporter::ImportScene(const AssetMetadata& metadata)
	{
		Ref<Scene> scene = CreateRef<Scene>();
		SceneSerializer::Deserialize(scene, metadata.Path);
		return scene;
	}
}
