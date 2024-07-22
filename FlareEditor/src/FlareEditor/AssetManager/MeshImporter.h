#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	struct AssetMetadata;
	class Mesh;
	class MeshImporter
	{
	public:
		static Ref<Mesh> ImportMesh(const AssetMetadata& metadata);
	};
}