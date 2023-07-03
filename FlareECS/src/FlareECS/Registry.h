#pragma once

#include "Flare/Core/Assert.h"
#include "Flare/Core/Core.h"

#include "FlareECS/Entity.h"
#include "FlareECS/Component.h"
#include "FlareECS/Archetype.h"

#include "FlareECS/Query/QueryCache.h"

#include <unordered_map>
#include <vector>
#include <optional>

namespace Flare
{
	class EntityView;
	class EntityArchetypesView;
	class EntityRegistryIterator;

	class Registry
	{
	public:
		Registry()
			: m_QueryCache(*this) {}

	public:
		// Entity operations

		Entity CreateEntity(ComponentSet& components);
		bool AddEntityComponent(Entity entity, ComponentId componentId, const void* componentData);
		bool RemoveEntityComponent(Entity entity, ComponentId componentId);

		// Component operations

		ComponentId RegisterComponent(std::string_view name, size_t size);

		std::optional<void*> GetEntityComponent(Entity entity, ComponentId component);
		const ComponentSet& GetEntityComponents(Entity entity);
		bool HasComponent(Entity entity, ComponentId component);

		inline const ComponentInfo& GetComponentInfo(size_t index) const;
		inline const std::vector<ComponentInfo>& GetRegisteredComponents() const { return m_RegisteredComponents; }

		// Archetype operations

		inline ArchetypeRecord& GetArchetypeRecord(size_t archetypeId) { return m_Archetypes[archetypeId]; }
		std::optional<size_t> GetArchetypeComponentIndex(ArchetypeId archetype, ComponentId component);

		// Iterator

		EntityRegistryIterator begin();
		EntityRegistryIterator end();

		// Querying

		QueryId CreateQuery(const ComponentSet& components);

		EntityView QueryArchetype(ComponentSet components);
		EntityArchetypesView ExecuteQuery(QueryId query);
	public:
		inline EntityRecord& operator[](size_t index);
		inline const EntityRecord& operator[](size_t index) const;
	private:
		ArchetypeId CreateArchetype();

		void RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex);
	private:
		std::vector<EntityRecord> m_EntityRecords;
		std::unordered_map<Entity, size_t> m_EntityToRecord;

		std::vector<ArchetypeRecord> m_Archetypes;
		std::unordered_map<ComponentSet, size_t> m_ComponentSetToArchetype;

		std::unordered_map<ComponentId, std::unordered_map<ArchetypeId, size_t>> m_ComponentToArchetype;

		std::vector<ComponentInfo> m_RegisteredComponents;

		QueryCache m_QueryCache;

		friend class EntityRegistryIterator;
		friend class QueryCache;
	};
}