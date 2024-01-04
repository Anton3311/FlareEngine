#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class FLARE_API MaterialsTable : public Asset
	{
	public:
		MaterialsTable();
	public:
		std::vector<AssetHandle> Materials;
	};
}
