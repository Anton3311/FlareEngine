#pragma once

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Query/QueryFilters.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	using QueryId = size_t;
	constexpr QueryId INVALID_QUERY_ID = SIZE_MAX;

	enum class QueryTarget : uint8_t
	{
		AllEntities,
		DeletedEntities,
		CreatedEntities,
	};

	struct QueryData
	{
		struct ComponentEntry
		{
			ComponentId Id = ComponentId();
			QueryFilterType Filter = QueryFilterType::With;
		};

		QueryId Id;
		QueryTarget Target;

		std::vector<ComponentEntry> Components;
		std::unordered_set<ArchetypeId> MatchedArchetypes;
	};

	struct QueryCreationData
	{
		QueryTarget Target;
		std::vector<QueryData::ComponentEntry> Components;
	};
}