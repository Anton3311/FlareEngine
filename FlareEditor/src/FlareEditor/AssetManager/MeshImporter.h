#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/Mesh.h"

namespace Flare
{
	class MeshImporter
	{
	public:
		static Ref<Mesh> ImportMesh(const AssetMetadata& metadata);
	};
}