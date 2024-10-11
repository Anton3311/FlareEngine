#include "PCH.h"

#include "MeshSource.h"

namespace Flare
{
	FLARE_SERIALIZABLE_IMPL(MeshSource);
	FLARE_IMPL_ASSET(MeshSource);

	MeshSource::MeshSource()
		: Asset(AssetType::MeshSource)
	{
	}
}
