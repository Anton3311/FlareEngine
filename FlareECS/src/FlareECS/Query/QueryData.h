#pragma once

#include "FlareECS/Component.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	struct QueryData
	{
		std::vector<ComponentId> Components;
		std::unordered_set<ArchetypeId> MatchedArchetypes;
	};
}