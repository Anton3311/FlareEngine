#include "MaterialsTable.h"

namespace Flare
{
	FLARE_IMPL_ASSET(MaterialsTable);
	FLARE_SERIALIZABLE_IMPL(MaterialsTable);

	MaterialsTable::MaterialsTable()
		: Asset(AssetType::MaterialsTable)
	{
	}
}
