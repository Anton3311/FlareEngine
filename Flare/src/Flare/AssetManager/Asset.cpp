#include "Asset.h"

namespace Flare
{
	FLARE_IMPL_TYPE(AssetHandle);

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

	AssetType AssetTypeFromString(std::string_view string)
	{
		if (string == "Texture")
			return AssetType::Texture;
		else if (string == "Scene")
			return AssetType::Scene;

		FLARE_CORE_ASSERT(false, "Unknown asset type string");
		return AssetType::None;
	}
}
