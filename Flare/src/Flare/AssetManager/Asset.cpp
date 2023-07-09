#include "Asset.h"

namespace Flare
{
	std::string_view AssetTypeToString(AssetType type)
	{
		switch (type)
		{
		case AssetType::None:
			return "None";
		case AssetType::Scene:
			return "Scene";
		case AssetType::Texture:
			return "Texture";
		}

		FLARE_CORE_ASSERT(false, "Unhandled asset type");
		return "";
	}
}
