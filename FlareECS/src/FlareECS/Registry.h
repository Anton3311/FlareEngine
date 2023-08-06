#pragma once

#include "Flare/Core/Assert.h"
#include "Flare/Core/Core.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Archetype.h"
#include "FlareECS/Entity/EntityIndex.h"

#include "FlareECS/Query/QueryCache.h"

#include <unordered_map>
#include <vector>
#include <optional>

namespace Flare
{
	class Query;

	class EntityView;
	class EntityRegistryIterator;

	class Registry
	{
	public:
		Registry();
		~Registry();
	public:
		// Entity operations

		Entity CreateEntity(const ComponentSet& componentSet);
		void DeleteEntity(Entity entity);
		bool AddEntityComponent(Entity entity, ComponentId componentId, const void* componentData);
		bool RemoveEntityComponent(Entity entity, ComponentId componentId);
		bool IsEntityAlive(Entity entity) const;

		inline const std::vector<EntityRecord>& GetEntityRecords() const { return m_EntityRecords; }

		std::optional<Entity> FindEntityByIndex(uint32_t entityIndex);
		std::optional<Entity> FindEntityByRegistryIndex(uint32_t registryIndex);

		std::optional<uint8_t*> GetEntityData(Entity entity);
		std::optional<const uint8_t*> GetEntityData(Entity entity) const;
		std::optional<size_t> GetEntityDataSize(Entity entity);

		// Component operations

		ComponentId RegisterComponent(std::string_view name, size_t size, const std::function<void(void*)>& deleter);
		std::optional<ComponentId> FindComponnet(std::string_view name);
		bool IsComponentIdValid(ComponentId id) const;

		std::optional<void*> GetEntityComponent(Entity entity, ComponentId component);
		const std::vector<ComponentId>& GetEntityComponents(Entity entity);
		bool HasComponent(Entity entity, ComponentId component) const;

		inline const ComponentInfo& GetComponentInfo(ComponentId id) const;
		inline const std::vector<ComponentInfo>& GetRegisteredComponents() const { return m_RegisteredComponents; }

		std::optional<void*> GetSingleComponent(ComponentId id) const;
		std::optional<Entity> GetSingletonEntity(const Query& query) const;
		
		// Archetype operations

		inline ArchetypeRecord& GetArchetypeRecord(size_t archetypeId) { return m_Archetypes[archetypeId]; }
		inline const ArchetypeRecord& GetArchetypeRecord(size_t archetypeId) const { return m_Archetypes[archetypeId]; }
		std::optional<size_t> GetArchetypeComponentIndex(ArchetypeId archetype, ComponentId component) const;

		// Iterator

		EntityRegistryIterator begin();
		EntityRegistryIterator end();

		// Querying

		Query CreateQuery(const ComponentSet& components);
		inline const QueryCache& GetQueryCache() const { return m_QueryCache; }

		EntityView QueryArchetype(const ComponentSet& componentSet);
	public:
		inline EntityRecord& operator[](size_t index);
		inline const EntityRecord& operator[](size_t index) const;
	private:
		ArchetypeId CreateArchetype();

		void RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex);

		std::unordered_map<Entity, size_t>::iterator FindEntity(Entity entity);
		std::unordered_map<Entity, size_t>::const_iterator FindEntity(Entity entity) const;
	private:
		std::vector<ComponentId> m_TemporaryComponentSet;

		std::vector<EntityRecord> m_EntityRecords;
		std::unordered_map<Entity, size_t> m_EntityToRecord;

		std::vector<ArchetypeRecord> m_Archetypes;
		std::unordered_map<ComponentSet, size_t> m_ComponentSetToArchetype;

		std::unordered_map<ComponentId, std::unordered_map<ArchetypeId, size_t>> m_ComponentToArchetype;

		std::unordered_map<std::string_view, uint32_t> m_ComponentNameToIndex;
		std::unordered_map<ComponentId, uint32_t> m_ComponentIdToIndex;
		std::vector<ComponentInfo> m_RegisteredComponents;

		QueryCache m_QueryCache;
		EntityIndex m_EntityIndex;

		friend class EntityRegistryIterator;
		friend class QueryCache;
	};
}