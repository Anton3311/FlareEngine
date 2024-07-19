#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/Material.h"

namespace Flare
{
	class MaterialImporter
	{
	public:
		static void SerializeMaterial(Ref<Material> material, const std::filesystem::path& path);

		static Ref<Material> ImportMaterial(const AssetMetadata& metadata);
	};
}