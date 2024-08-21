#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class FLARE_API MeshSource : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		MeshSource();
	};
}