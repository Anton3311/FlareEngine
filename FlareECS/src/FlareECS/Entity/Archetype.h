#pragma once

#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Entity/Component.h"

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
		ArchetypeRecord()
			: Id(INVALID_ARCHETYPE_ID), DeletionQueryReferences(0), CreatedEntitiesQueryReferences(0) {}

		ArchetypeRecord(const ArchetypeRecord&) = delete;

		ArchetypeRecord(ArchetypeRecord&& other) noexcept = default;

		ArchetypeRecord& operator=(const ArchetypeRecord&) = delete;
		ArchetypeRecord& operator=(ArchetypeRecord&& other) noexcept = default;

		std::optional<size_t> TryGetComponentIndex(ComponentId component) const;

		constexpr bool IsUsedInDeletionQuery() const { return DeletionQueryReferences > 0; }
		constexpr bool IsUsedInCreatedEntitiesQuery() const { return CreatedEntitiesQueryReferences > 0; }

		ArchetypeId Id;
		size_t EntitySize = 0;
		size_t EntityAlignment = 0;
		int32_t DeletionQueryReferences = 0;
		int32_t CreatedEntitiesQueryReferences = 0;

		TypeFlags CombinedComponentTypeFlags = TypeFlags::None;
		
		std::vector<ComponentId> Components; // Sorted
		std::vector<size_t> ComponentOffsets;

		std::vector<size_t> ComponentArrayOffsets;

		std::unordered_map<ComponentId, ArchetypeEdge> Edges;
	};
}