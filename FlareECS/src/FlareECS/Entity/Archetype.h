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

	struct FLAREECS_API Archetype
	{
		size_t Id;

		std::vector<ComponentId> Components; // Sorted
		std::vector<size_t> ComponentOffsets;

		std::unordered_map<ComponentId, ArchetypeEdge> Edges;

		Archetype()
			: Id(INVALID_ARCHETYPE_ID) {}

		Archetype(Archetype&& other) noexcept
			: Id(other.Id), Components(std::move(other.Components)), ComponentOffsets(std::move(other.ComponentOffsets)), Edges(std::move(other.Edges))
		{
			other.Id = INVALID_ARCHETYPE_ID;
		}
	};
	
	struct FLAREECS_API ArchetypeRecord
	{
		Archetype Data;
		EntityStorage Storage;

		ArchetypeRecord() {}

		ArchetypeRecord(const ArchetypeRecord& other)
		{
			FLARE_CORE_INFO("ArchetypeRecord::Copy");
		}

		ArchetypeRecord(ArchetypeRecord&& other) noexcept
			: Data(std::move(other.Data)), Storage(std::move(other.Storage)) {}
	};
}