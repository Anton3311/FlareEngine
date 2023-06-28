#pragma once

#include "FlareECS/Component.h"
#include "FlareECS/EntityStorage.h"

#include <vector>

namespace Flare
{
	struct Archetype
	{
		size_t Id;
		std::vector<ComponentId> Components; // Sorted
		std::vector<size_t> ComponentOffsets;
	};
	
	struct ArchetypeRecord
	{
		Archetype Data;
		EntityStorage Storage;
	};
}