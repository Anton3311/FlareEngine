#pragma once

#include "FlareECS/Entity/Archetype.h"
#include "FlareECS/Entity/Component.h"
#include "FlareECS/QueryFilters.h"
#include "FlareECS/QueryId.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	struct QueryData
	{
		QueryId Id;

		std::vector<ComponentId> Components;
		std::unordered_set<ArchetypeId> MatchedArchetypes;
	};
}