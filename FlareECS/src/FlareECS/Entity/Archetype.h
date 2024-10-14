#pragma once

#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Types.h"
#include "FlareECS/Entity/Component.h"

#include <vector>
#include <optional>
#include <unordered_map>

namespace Flare
{
	using ArchetypeId = uint32_t;
	constexpr ArchetypeId INVALID_ARCHETYPE_ID = std::numeric_limits<ArchetypeId>::max();

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
		EntitySizeT EntitySize = 0;
		EntitySizeT EntityAlignment = 0;
		EntitySizeT EntityCountPerChunk = 0;

		int32_t DeletionQueryReferences = 0;
		int32_t CreatedEntitiesQueryReferences = 0;

		TypeFlags CombinedComponentTypeFlags = TypeFlags::None;
		
		std::vector<ComponentId> Components; // Sorted
		std::vector<EntitySizeT> ComponentOffsets;

		std::vector<EntitySizeT> ComponentArrayOffsets;
		EntitySizeT IdsBufferOffset = 0;

		std::unordered_map<ComponentId, ArchetypeEdge> Edges;
	};
}