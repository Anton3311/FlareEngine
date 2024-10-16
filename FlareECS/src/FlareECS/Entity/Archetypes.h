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
		ArchetypeId Add = INVALID_ARCHETYPE_ID;
		ArchetypeId Remove = INVALID_ARCHETYPE_ID;
	};

	class FLAREECS_API ArchetypeRecord
	{
	public:
		FLARE_NONCOPYABLE(ArchetypeRecord);

		ArchetypeRecord() = default;

		ArchetypeRecord(ArchetypeRecord&&) = default;
		ArchetypeRecord& operator=(ArchetypeRecord&&) = default;

		std::optional<size_t> TryGetComponentIndex(ComponentId component) const;

		constexpr bool IsUsedInDeletionQuery() const { return DeletionQueryReferences > 0; }
		constexpr bool IsUsedInCreatedEntitiesQuery() const { return CreatedEntitiesQueryReferences > 0; }
	public:
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

	//
	// ArchetypeUpdateHandler
	//

	class ArchetypeUpdateHandler
	{
	public:
		virtual void OnArchetypeCreated(ArchetypeId id) = 0;
	};

	//
	// Archetypes
	//

	struct Components;
	class FLAREECS_API Archetypes
	{
	public:
		FLARE_NONCOPYABLE(Archetypes);

		Archetypes(const Components& componentsRegistry);
		~Archetypes();

		void Clear();

		constexpr size_t GetArchetypeCount() const { return m_Records.size(); }

		inline bool IsIdValid(ArchetypeId id) const
		{
			return (size_t)id < m_Records.size();
		}

		inline const ArchetypeRecord& operator[](ArchetypeId id) const
		{
			FLARE_CORE_ASSERT((size_t)id < m_Records.size());
			return m_Records[(size_t)id];
		}

		ArchetypeRecord& GetMutableRecord(ArchetypeId id)
		{
			FLARE_CORE_ASSERT(IsIdValid(id));
			return m_Records[id];
		}

		Span<const ArchetypeRecord> GetRecords() const { return Span(m_Records.data(), m_Records.size()); }

		const ArchetypeRecord* FindArchetype(Span<const ComponentId> components) const;
		const ArchetypeRecord* FindOrCreateArchetype(Span<const ComponentId> components);
		
		ArchetypeId CreateArchetype(Span<const ComponentId> sortedComponentIds);
		ArchetypeId CreateArchetype(std::vector<ComponentId>&& sortedComponentIds);

		void AddUpdateHandler(ArchetypeUpdateHandler* handler);
		void RemoveUpdateHandler(ArchetypeUpdateHandler* handler);

		inline const Components& GetCompatibleComponents() const { return m_ComponentsRegistry; }

		void AddArchetypeEdge(ArchetypeId archetype, ComponentId component, ArchetypeEdge edge);

		const std::unordered_map<ArchetypeId, size_t>* GetArchetypesWithComponent(ComponentId component) const
		{
			auto it = m_ComponentToArchetype.find(component);
			if (it == m_ComponentToArchetype.end())
				return nullptr;

			return &it->second;
		}
	private:
		void InitializeRecord(ArchetypeRecord& archetype);
	private:
		const Components& m_ComponentsRegistry;

		std::vector<ArchetypeRecord> m_Records;
		std::unordered_map<ComponentSet, ArchetypeId> m_ComponentSetToArchetype;
		std::unordered_map<ComponentId, std::unordered_map<ArchetypeId, size_t>> m_ComponentToArchetype;

		std::vector<ArchetypeUpdateHandler*> m_UpdateHandlers;
	};
}