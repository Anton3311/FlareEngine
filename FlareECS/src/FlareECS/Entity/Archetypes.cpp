#include "Archetypes.h"

namespace Flare
{
	Archetypes::~Archetypes()
	{
		for (const auto& archetype : Records)
		{
			FLARE_CORE_ASSERT(archetype.DeletionQueryReferences == 0 && archetype.CreatedEntitiesQueryReferences == 0);
		}
	}
}
