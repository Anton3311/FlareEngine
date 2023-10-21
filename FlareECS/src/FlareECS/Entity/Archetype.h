#pragma once

#include "FlareECS/Entity/Component.h"
#include "FlareECS/EntityStorage/EntityStorage.h"

#include <vector>
#include <optional>
#include <unordered_map>

namespace Flare
{
	using ArchetypeId = size_t;
	constexpr ArchetypeId INVALID_ARCHETYPE_ID = SIZE_MAX;

	struct ArchetypeEdge
	{
		ArchetypeId Add;
		ArchetypeId Remove;
	};

	struct FLAREECS_API ArchetypeRecord
	{
		size_t Id;
		
		std::vector<ComponentId> Components; // Sorted
		std::vector<size_t> ComponentOffsets;

		std::unordered_map<ComponentId, ArchetypeEdge> Edges;

		ArchetypeRecord()
			: Id(INVALID_ARCHETYPE_ID) {}

		ArchetypeRecord(const ArchetypeRecord&) = delete;

		ArchetypeRecord(ArchetypeRecord&& other) noexcept
			: Id(other.Id),
			Components(std::move(other.Components)),
			ComponentOffsets(std::move(other.ComponentOffsets)),
			Edges(std::move(other.Edges))
		{
			other.Id = INVALID_ARCHETYPE_ID;
		}

		ArchetypeRecord& operator=(const ArchetypeRecord&) = delete;
		ArchetypeRecord& operator=(ArchetypeRecord&& other) noexcept
		{
			Id = other.Id;
			Components = std::move(other.Components);
			ComponentOffsets = std::move(other.ComponentOffsets);
			Edges = std::move(other.Edges);

			other.Id = INVALID_ARCHETYPE_ID;
			return *this;
		}
	};
}