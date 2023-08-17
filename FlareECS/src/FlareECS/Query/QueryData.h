#pragma once

#include "FlareECS/Entity/Archetype.h"
#include "FlareECS/Entity/Component.h"
#include "FlareECS/Query/QueryFilters.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	using QueryId = size_t;
	constexpr QueryId INVALID_QUERY_ID = SIZE_MAX;

	struct QueryData
	{
		QueryId Id;

		std::vector<ComponentId> Components;
		std::unordered_set<ArchetypeId> MatchedArchetypes;
	};
}