#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Collections/Span.h"

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
		FLARE_NONCOPYABLE(ArchetypeRecord);

		ArchetypeRecord() = default;

		ArchetypeRecord(ArchetypeRecord&&) = default;
		ArchetypeRecord& operator=(ArchetypeRecord&&) = default;

		std::optional<size_t> TryGetComponentIndex(ComponentId component) const;

		constexpr bool IsUsedInDeletionQuery() const { return DeletionQueryReferences > 0; }
		constexpr bool IsUsedInCreatedEntitiesQuery() const { return CreatedEntitiesQueryReferences > 0; }

		ArchetypeId Id = INVALID_ARCHETYPE_ID;
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

	class ArchetypeUpdateHandler
	{
	public:
		virtual void OnArchetypeCreated(ArchetypeId id) = 0;
	};

	struct Components;
	struct FLAREECS_API Archetypes
	{
		Archetypes(const Components& componentsRegistry);
		~Archetypes();

		Archetypes(const Archetypes&) = delete;
		Archetypes& operator=(const Archetypes&) = delete;

		inline void Clear()
		{
			ComponentSetToArchetype.clear();
			ComponentToArchetype.clear();

			Records.clear();
		}

		inline bool IsIdValid(ArchetypeId id) const
		{
			return (size_t)id < Records.size();
		}

		inline const ArchetypeRecord& operator[](ArchetypeId id) const
		{
			FLARE_CORE_ASSERT((size_t)id < Records.size());
			return Records[(size_t)id];
		}

		const ArchetypeRecord* FindArchetype(Span<ComponentId> components) const;
		const ArchetypeRecord* FindOrCreateArchetype(Span<ComponentId> components);
		
		ArchetypeId CreateArchetype(Span<const ComponentId> sortedComponentIds);
		ArchetypeId CreateArchetype(std::vector<ComponentId>&& sortedComponentIds);

		void AddUpdateHandler(ArchetypeUpdateHandler* handler);
		void RemoveUpdateHandler(ArchetypeUpdateHandler* handler);

		inline const Components& GetCompatibleComponents() const { return m_ComponentsRegistry; }
	private:
		void InitializeRecord(ArchetypeRecord& archetype);
	public:
		std::vector<ArchetypeRecord> Records;
		std::unordered_map<ComponentSet, ArchetypeId> ComponentSetToArchetype;
		std::unordered_map<ComponentId, std::unordered_map<ArchetypeId, size_t>> ComponentToArchetype;
	private:
		const Components& m_ComponentsRegistry;

		std::vector<ArchetypeUpdateHandler*> m_UpdateHandlers;
	};
}