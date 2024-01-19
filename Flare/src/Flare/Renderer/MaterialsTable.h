#pragma once

#include "Flare/AssetManager/Asset.h"
#include "FlareCore/Serialization/Metadata.h"

namespace Flare
{
	class FLARE_API MaterialsTable : public Asset
	{
	public:
		FLARE_ASSET;
		FLARE_SERIALIZABLE;

		MaterialsTable();
	public:
		std::vector<AssetHandle> Materials;
	};
}
