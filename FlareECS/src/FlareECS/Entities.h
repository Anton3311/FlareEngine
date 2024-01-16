#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Core.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/Entity/Archetype.h"
#include "FlareECS/Entity/Archetypes.h"
#include "FlareECS/Entity/EntityIndex.h"

#include "FlareECS/EntityStorage/EntityStorage.h"
#include "FlareECS/EntityStorage/DeletedEntitiesStorage.h"

#include "FlareECS/Query/QueryCache.h"

#include <unordered_map>
#include <vector>
#include <optional>

namespace Flare
{
	class FLAREECS_API Query;

	class EntityView;
	class EntitiesIterator;

	enum class ComponentInitializationStrategy : uint8_t
	{
		Zero,
		DefaultConstructor,
	};

	class FLAREECS_API Entities
	{
	public:
		Entities(Components& components, QueryCache& queries, Archetypes& archetypes);
		Entities(const Entities&) = delete;

		Entities& operator=(const Entities&) = delete;

		~Entities();

		void Clear();
	public:
		// Entity operations

		Entity CreateEntity(const ComponentSet& componentSet, 
			ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor);

		// Components must be sorted by [ComponentId]
		Entity CreateEntity(const std::pair<ComponentId, void*>* components, size_t count, bool copyComponents = false);
		
		Entity CreateEntityFromArchetype(ArchetypeId archetype,
			ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor);

		void DeleteEntity(Entity entity);

		bool AddEntityComponent(Entity entity, 
			ComponentId componentId, 
			void* componentData, 
			ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor);

		bool RemoveEntityComponent(Entity entity, ComponentId componentId);
		bool IsEntityAlive(Entity entity) const;

		ArchetypeId GetEntityArchetype(Entity entity);

		const std::vector<EntityRecord>& GetEntityRecords() const;

		std::optional<Entity> FindEntityByIndex(uint32_t entityIndex);
		std::optional<Entity> FindEntityByRegistryIndex(uint32_t registryIndex);

		std::optional<uint8_t*> GetEntityData(Entity entity);
		std::optional<const uint8_t*> GetEntityData(Entity entity) const;
		std::optional<size_t> GetEntityDataSize(Entity entity) const;

		EntityStorage& GetEntityStorage(ArchetypeId archetype);
		const EntityStorage& GetEntityStorage(ArchetypeId archetype) const;

		DeletedEntitiesStorage& GetDeletedEntityStorage(ArchetypeId archetype);
		const DeletedEntitiesStorage& GetDeletedEntityStorage(ArchetypeId archetype) const;

		void ClearQueuedForDeletion();

		// Component operations

		void* GetEntityComponent(Entity entity, ComponentId component);
		const void* GetEntityComponent(Entity entity, ComponentId component) const;

		const std::vector<ComponentId>& GetEntityComponents(Entity entity);
		bool HasComponent(Entity entity, ComponentId component) const;

		void* GetSingletonComponent(ComponentId id) const;
		std::optional<Entity> GetSingletonEntity(const Query& query) const;

		// Archetypes

		inline const Archetypes& GetArchetypes() const { return m_Archetypes; }
		inline const Components& GetComponents() const { return m_Components; }
		
		// Iterator

		EntitiesIterator begin();
		EntitiesIterator end();
	public:
		EntityRecord& operator[](size_t index);
		const EntityRecord& operator[](size_t index) const;
	private:
		struct EntityCreationResult
		{
			Entity Id;
			ArchetypeId Archetype = INVALID_ARCHETYPE_ID;
			uint8_t* Data = nullptr;
		};

		void CreateEntity(const ComponentSet& components, EntityCreationResult& result);
		void InitializeEntity(const EntityCreationResult& entityResult, ComponentInitializationStrategy initStrategy);

		ArchetypeId CreateArchetype();

		void RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex);
		void ValidateEntityStorages();

		std::unordered_map<Entity, size_t>::iterator FindEntity(Entity entity);
		std::unordered_map<Entity, size_t>::const_iterator FindEntity(Entity entity) const;
	private:
		std::vector<ComponentId> m_TemporaryComponentSet;

		Archetypes& m_Archetypes;
		QueryCache& m_Queries;
		Components& m_Components;

		std::vector<EntityStorage> m_EntityStorages;
		std::vector<DeletedEntitiesStorage> m_DeletedEntitiesStorages;

		std::vector<EntityRecord> m_EntityRecords;
		std::unordered_map<Entity, size_t> m_EntityToRecord;

		EntityIndex m_EntityIndex;

		friend class EntitiesIterator;
		friend class QueryCache;
	};
}