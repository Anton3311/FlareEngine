#pragma once

#include "FlareECS/Component.h"
#include "FlareECS/EntityStorage.h"

#include <vector>
#include <optional>

namespace Flare
{
	struct Archetype
	{
		size_t Id;
		std::vector<ComponentId> Components; // Sorted
		std::vector<size_t> ComponentOffsets;

		std::optional<size_t> FindComponent(ComponentId component);
	};
	
	struct ArchetypeRecord
	{
		Archetype Data;
		EntityStorage Storage;
	};
}