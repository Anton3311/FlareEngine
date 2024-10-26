#include "Entities.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Query/EntitiesIterator.h"

#include "FlareECS/Query/Query.h"

#include "FlareECS/Entity/ComponentInitializer.h"

#include "FlareECS/EntityStorage/EntityStorage.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <algorithm>

namespace Flare
{
	Entities::Entities(Components& components, Archetypes& archetypes)
		: m_Components(components), m_Archetypes(archetypes)
	{
		FLARE_PROFILE_FUNCTION();
		EntityChunksPool::Initialize(16);

		m_Archetypes.AddUpdateHandler(this);

		EnsureValidEntityStorages();
	}

	Entities::~Entities()
	{
		m_Archetypes.RemoveUpdateHandler(this);
		ReleaseEntityData();
	}

	void Entities::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		ReleaseEntityData();

		for (const EntityRecord& record : m_EntityRecords)
			m_EntityIndex.AddDeletedId(record.Id);

		m_EntityStorages.clear();
		m_EntityRecords.clear();
		m_EntityToRecord.clear();
		m_TemporaryComponentSet.clear();
	}

	Entity Entities::CreateEntity(const ComponentSet& componentSet, ComponentInitializationStrategy initStrategy)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(componentSet.GetSize() > 0);

		if (m_TemporaryComponentSet.size() < componentSet.GetSize())
			m_TemporaryComponentSet.resize(componentSet.GetSize());

		std::memcpy(m_TemporaryComponentSet.data(), componentSet.GetData(), sizeof(ComponentId) * componentSet.GetSize());

		std::sort(m_TemporaryComponentSet.data(), m_TemporaryComponentSet.data() + componentSet.GetSize());

		ComponentSet components = ComponentSet(m_TemporaryComponentSet.data(), componentSet.GetSize());

		EntityCreationResult result;
		CreateEntity(components, result);

		const auto& archetype = m_Archetypes[result.Archetype];
		const EntityRecord& entityRecord = m_EntityRecords[FindEntity(result.Id)->second];

		GetEntityStorage(result.Archetype).DefaultConstructEntity(entityRecord.BufferIndex);

		return result.Id;
	}

	Entity Entities::CreateEntity(const std::pair<ComponentId, void*>* components, size_t count, bool copyComponents)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_TemporaryComponentSet.size() < count)
			m_TemporaryComponentSet.resize(count);

		for (size_t i = 0; i < count; i++)
			m_TemporaryComponentSet[i] = components[i].first;

		EntityCreationResult result;
		CreateEntity(ComponentSet(m_TemporaryComponentSet.data(), count), result);

		const auto& archetype = m_Archetypes[result.Archetype];
		const EntityRecord& entityRecord = m_EntityRecords[FindEntity(result.Id)->second];

		if (copyComponents)
		{
			std::vector<const void*> componentData;
			componentData.resize(count);

			for (size_t i = 0; i < count; i++)
				componentData[i] = components[i].second;

			GetEntityStorage(result.Archetype).CopyConstructEntity(entityRecord.BufferIndex, Span<const void*>::FromVector(componentData));
		}
		else
		{
			std::vector<void*> componentData;
			componentData.resize(count);

			for (size_t i = 0; i < count; i++)
				componentData[i] = components[i].second;

			GetEntityStorage(result.Archetype).MoveConstructEntity(entityRecord.BufferIndex, Span<void*>::FromVector(componentData));
		}

		return result.Id;
	}

	Entity Entities::CreateEntityFromArchetype(ArchetypeId archetype, ComponentInitializationStrategy initStrategy)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Archetypes.IsIdValid(archetype));

		const ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];
		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(archetype);
		EntityCreationResult entityResult{};
		CreateEntity(archetypeRecord, entityResult);

		const EntityRecord& entityRecord = m_EntityRecords[FindEntity(entityResult.Id)->second];

		switch (initStrategy)
		{
		case ComponentInitializationStrategy::NoInitialization:
			break;
		case ComponentInitializationStrategy::DefaultConstructor:
		{
			EntityStorage& storage = GetEntityStorage(archetype);
			for (EntitySizeT componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
			{
				void* componentData = storage.GetEntityComponentData(entityRecord.BufferIndex, componentIndex);
				m_Components.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]).Initializer->Type.Functions.DefaultConstructor(componentData);
			}

			break;
		}
		default:
			FLARE_VERIFY_UNREACHABLE();
		}

		return entityResult.Id;
	}

	void Entities::DeleteEntity(Entity entity, bool ignoreDeletionQueries)
	{
		FLARE_PROFILE_FUNCTION();
		auto recordIterator = FindEntity(entity);
		if (recordIterator == m_EntityToRecord.end())
			return;

		EntityRecord& record = m_EntityRecords[recordIterator->second];
		EntityRecord& lastEntityRecord = m_EntityRecords.back();

		EntityStorage& storage = GetEntityStorage(record.Archetype);

		Entity lastEntityInBuffer = storage.GetEntityId(storage.GetEntityCount() - 1);
		if (lastEntityInBuffer != entity)
		{
			auto it = m_EntityToRecord.find(lastEntityInBuffer);
			EntityRecord& lastEntityInBufferRecord = m_EntityRecords[it->second];
			lastEntityInBufferRecord.BufferIndex = record.BufferIndex;
		}

		const ArchetypeRecord& archetype = m_Archetypes[record.Archetype];
		if (!ignoreDeletionQueries && archetype.IsUsedInDeletionQuery())
		{
			auto& deletedEntities = GetDeletedEntityStorage(archetype.Id);

			size_t index = deletedEntities.AddEntity(entity);

			deletedEntities.MoveEntityData(record.BufferIndex, storage, index);
		}

		storage.RemoveEntity(record.BufferIndex);

		m_EntityIndex.AddDeletedId(record.Id);
		m_EntityToRecord.erase(record.Id);

		lastEntityRecord.RegistryIndex = record.RegistryIndex;
		record = lastEntityRecord;

		if (lastEntityRecord.Id != entity)
		{
			m_EntityToRecord[lastEntityRecord.Id] = lastEntityRecord.RegistryIndex;
		}

		m_EntityRecords.erase(m_EntityRecords.end() - 1);
	}

	bool Entities::AddEntityComponent(Entity entity, ComponentId componentId, void* componentData, ComponentInitializationStrategy initStrategy)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(componentId), "Invalid component id");

		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(componentId);

		auto recordIterator = FindEntity(entity);
		if (recordIterator == m_EntityToRecord.end())
			return false;

		EntityRecord& entityRecord = m_EntityRecords[recordIterator->second];
		const ArchetypeRecord& archetype = m_Archetypes[entityRecord.Archetype];
		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(entityRecord.Archetype);

		// Can only have one instance of a component
		EntitySizeT componentIndex = archetypeComponents.TryGetComponentIndex(componentId);
		if (componentIndex != ArchetypeComponents::INVALID_COMPONENT_INDEX)
			return false;

		size_t oldComponentCount = archetypeComponents.ComponentCount;
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;

		size_t insertedComponentIndex = SIZE_MAX;

		auto edgeIterator = archetype.Edges.find(componentId);
		if (edgeIterator != archetype.Edges.end())
		{
			newArchetypeId = edgeIterator->second.Add;

			const ArchetypeComponents& newArchetypeComponents = m_Archetypes.GetArchetypeComponents(newArchetypeId);
			
			EntitySizeT index = newArchetypeComponents.TryGetComponentIndex(componentId);
			if (index != ArchetypeComponents::INVALID_COMPONENT_INDEX)
				insertedComponentIndex = index;
			else
			{
				FLARE_CORE_ASSERT(false, "Archetype doesn't have a component because archetype graph has invalid edge connection");
				return false;
			}
		}
		else
		{
			FLARE_PROFILE_SCOPE("FindOrCreateArchetype");
			std::vector<ComponentId> newComponents(oldComponentCount + 1);

			std::memcpy(newComponents.data(), archetypeComponents.ComponentIds, oldComponentCount * sizeof(componentId));
			newComponents[oldComponentCount] = componentId;

			insertedComponentIndex = oldComponentCount;
			for (size_t i = insertedComponentIndex; i > 0; i--)
			{
				if (newComponents[i - 1] > newComponents[i])
				{
					std::swap(newComponents[i - 1], newComponents[i]);
					insertedComponentIndex = i - 1;
				}
			}

			const ArchetypeRecord* foundArchetype = m_Archetypes.FindArchetype(Span<ComponentId>::FromVector(newComponents));
			if (foundArchetype)
			{
				newArchetypeId = foundArchetype->Id;
			}
			else
			{
				newArchetypeId = m_Archetypes.CreateArchetype(std::move(newComponents));
				const ArchetypeRecord& archetype = m_Archetypes[newArchetypeId];

				EntityStorage& storage = GetEntityStorage(newArchetypeId);

				EntityStorageRequirements storageRequirements{};
				storageRequirements.EntitySize = archetype.EntitySize;
				storageRequirements.EntityAlignment = archetype.EntityAlignment;
				storage.Initialize(storageRequirements, m_Archetypes, archetype.Id);
			}

			m_Archetypes.AddArchetypeEdge(entityRecord.Archetype, componentId, ArchetypeEdge{ newArchetypeId, INVALID_ARCHETYPE_ID });
			m_Archetypes.AddArchetypeEdge(newArchetypeId, componentId, ArchetypeEdge{ INVALID_ARCHETYPE_ID, entityRecord.Archetype });
		}

		FLARE_CORE_ASSERT(insertedComponentIndex != SIZE_MAX);

		const ArchetypeRecord& oldArchetype = m_Archetypes[entityRecord.Archetype];
		const ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
		const ArchetypeComponents& newArchetypeComponents = m_Archetypes.GetArchetypeComponents(newArchetypeId);

		EntityStorage& oldStorage = GetEntityStorage(oldArchetype.Id);
		EntityStorage& newStorage = GetEntityStorage(newArchetypeId);

		size_t oldEntityIndex = entityRecord.BufferIndex;
		size_t newEntityIndex = newStorage.AddEntity(entityRecord.Id);

		{
			// Copy construct an added component
			void* destination = newStorage.GetEntityComponentData(newEntityIndex, insertedComponentIndex);

			const ComponentInfo& componentInfo = m_Components.GetComponentInfo(componentId);

			if (componentData == nullptr)
			{
				componentInfo.Initializer->Type.Functions.DefaultConstructor(destination);
			}
			else
			{
				componentInfo.Initializer->Type.Functions.CopyConstructor(destination, componentData);
			}
		}

		{
			// Move construct components before the inserted one
			for (size_t componentIndex = 0; componentIndex < insertedComponentIndex; componentIndex++)
			{
				const ComponentInfo& componentInfo = m_Components.GetComponentInfo(newArchetypeComponents.ComponentIds[componentIndex]);

				componentInfo.Initializer->Type.Functions.MoveConstructor(
					newStorage.GetEntityComponentData(newEntityIndex, componentIndex),
					oldStorage.GetEntityComponentData(oldEntityIndex, componentIndex));
			}
		}

		{
			// Move construct components after the inserted one

			size_t sourceIndex = insertedComponentIndex;
			for (size_t destinationIndex = insertedComponentIndex + 1; destinationIndex < newArchetypeComponents.ComponentCount; destinationIndex++, sourceIndex++)
			{
				const ComponentInfo& componentInfo = m_Components.GetComponentInfo(newArchetypeComponents.ComponentIds[destinationIndex]);

				componentInfo.Initializer->Type.Functions.MoveConstructor(
					newStorage.GetEntityComponentData(newEntityIndex, destinationIndex),
					oldStorage.GetEntityComponentData(oldEntityIndex, sourceIndex));
			}
		}

		// Remove old entity data
		RemoveEntityData(entityRecord.Archetype, entityRecord.BufferIndex);

		entityRecord.Archetype = newArchetypeId;
		entityRecord.BufferIndex = newEntityIndex;

		return true;
	}

	bool Entities::RemoveEntityComponent(Entity entity, ComponentId componentId)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(componentId), "Invalid component id");

		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(componentId);

		auto recordIterator = FindEntity(entity);
		if (recordIterator == m_EntityToRecord.end())
			return false;

		EntityRecord& entityRecord = m_EntityRecords[recordIterator->second];
		const ArchetypeRecord& archetype = m_Archetypes[entityRecord.Archetype];
		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(entityRecord.Archetype);

		size_t removedComponentIndex = SIZE_MAX;
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;
		
		{
			EntitySizeT componentIndex = archetypeComponents.TryGetComponentIndex(componentId);
			if (componentIndex != ArchetypeComponents::INVALID_COMPONENT_INDEX)
				removedComponentIndex = componentIndex;
			else
				return false;
		}

		auto edgeIterator = archetype.Edges.find(componentId);
		if (edgeIterator != archetype.Edges.end())
		{
			newArchetypeId = edgeIterator->second.Remove;
		}
		else
		{
			FLARE_PROFILE_SCOPE("FindOrCreateArchetype");
			size_t oldComponentCount = archetypeComponents.ComponentCount;
			std::vector<ComponentId> newComponents(oldComponentCount - 1);

			for (size_t insertIndex = 0, i = 0; i < oldComponentCount; i++)
			{
				if (archetypeComponents.ComponentIds[i] == componentId)
					continue;
				else
				{
					newComponents[insertIndex] = archetypeComponents.ComponentIds[i];
					insertIndex++;
				}
			}

			const ArchetypeRecord* foundArchetype = m_Archetypes.FindArchetype(Span<ComponentId>::FromVector(newComponents));
			if (foundArchetype)
			{
				newArchetypeId = foundArchetype->Id;
			}
			else
			{
				newArchetypeId = m_Archetypes.CreateArchetype(std::move(newComponents));
				const ArchetypeRecord& archetype = m_Archetypes[newArchetypeId];

				EntityStorage& storage = GetEntityStorage(newArchetypeId);

				EntityStorageRequirements storageRequirements{};
				storageRequirements.EntitySize = archetype.EntitySize;
				storageRequirements.EntityAlignment = archetype.EntityAlignment;
				storage.Initialize(storageRequirements, m_Archetypes, archetype.Id);
			}

			m_Archetypes.AddArchetypeEdge(entityRecord.Archetype, componentId, ArchetypeEdge{ INVALID_ARCHETYPE_ID, newArchetypeId });
			m_Archetypes.AddArchetypeEdge(newArchetypeId, componentId, ArchetypeEdge{ newArchetypeId, INVALID_ARCHETYPE_ID });
		}

		const ArchetypeRecord& oldArchetype = m_Archetypes[entityRecord.Archetype];
		const ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
		const ArchetypeComponents& newArchetypeComponents = m_Archetypes.GetArchetypeComponents(entityRecord.Archetype);

		EntityStorage& oldStorage = GetEntityStorage(oldArchetype.Id);
		EntityStorage& newStorage = GetEntityStorage(newArchetypeId);

		size_t sizeBefore = oldArchetype.ComponentOffsets[removedComponentIndex];
		size_t sizeAfter = oldStorage.GetEntitySize() - (sizeBefore + componentInfo.Size);

		size_t oldEntityIndex = entityRecord.BufferIndex;
		size_t newEntityIndex = newStorage.AddEntity(entityRecord.Id);

		{
			// Move construct components before the removed one
			for (size_t componentIndex = 0; componentIndex < removedComponentIndex; componentIndex++)
			{
				const ComponentInfo& componentInfo = m_Components.GetComponentInfo(newArchetypeComponents.ComponentIds[componentIndex]);

				componentInfo.Initializer->Type.Functions.MoveConstructor(
					newStorage.GetEntityComponentData(newEntityIndex, componentIndex),
					oldStorage.GetEntityComponentData(oldEntityIndex, componentIndex));
			}
		}

		{
			// Move construct components after the removed one

			size_t sourceIndex = removedComponentIndex + 1;
			for (size_t destinationIndex = removedComponentIndex; destinationIndex < newArchetypeComponents.ComponentCount; destinationIndex++, sourceIndex++)
			{
				const ComponentInfo& componentInfo = m_Components.GetComponentInfo(newArchetypeComponents.ComponentIds[destinationIndex]);

				componentInfo.Initializer->Type.Functions.MoveConstructor(
					newStorage.GetEntityComponentData(newEntityIndex, destinationIndex),
					oldStorage.GetEntityComponentData(oldEntityIndex, sourceIndex));
			}
		}

		RemoveEntityData(entityRecord.Archetype, entityRecord.BufferIndex);

		entityRecord.Archetype = newArchetypeId;
		entityRecord.BufferIndex = newEntityIndex;

		return true;
	}

	bool Entities::IsEntityAlive(Entity entity) const
	{
		return FindEntity(entity) != m_EntityToRecord.end();
	}

	ArchetypeId Entities::GetEntityArchetype(Entity entity)
	{
		auto it = m_EntityToRecord.find(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());

		return m_EntityRecords[it->second].Archetype;
	}

	const std::vector<EntityRecord>& Entities::GetEntityRecords() const
	{
		return m_EntityRecords;
	}

	std::optional<Entity> Entities::FindEntityByIndex(uint32_t entityIndex)
	{
		auto it = m_EntityToRecord.find(Entity(entityIndex, 0));
		if (it == m_EntityToRecord.end())
			return {};
		return it->first;
	}

	std::optional<Entity> Entities::FindEntityByRegistryIndex(uint32_t registryIndex)
	{
		if (registryIndex < m_EntityRecords.size())
			return m_EntityRecords[registryIndex].Id;
		return {};
	}

	std::optional<size_t> Entities::GetEntityDataSize(Entity entity) const
	{
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		const EntityRecord& record = m_EntityRecords[it->second];
		return GetEntityStorage(record.Archetype).GetEntitySize();
	}

	void* Entities::GetEntityComponent(Entity entity, ComponentId component)
	{
		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(component);
		return GetRawEntityComponent(entity, component, componentInfo.Size);
	}

	const void* Entities::GetEntityComponent(Entity entity, ComponentId component) const
	{
		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(component);
		return GetRawEntityComponent(entity, component, componentInfo.Size);
	}

	void* Entities::GetRawEntityComponent(Entity entity, ComponentId component, size_t componentSize) const
	{
		FLARE_PROFILE_FUNCTION();
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		const EntityRecord& entityRecord = m_EntityRecords[it->second];
		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(entityRecord.Archetype);
		const EntityStorage& storage = m_EntityStorages[entityRecord.Archetype];

		EntitySizeT componentIndex = archetypeComponents.TryGetComponentIndex(component);
		if (componentIndex == ArchetypeComponents::INVALID_COMPONENT_INDEX)
			return nullptr;

		size_t chunkIndex = entityRecord.BufferIndex / storage.GetEntitiesPerChunk();
		size_t indexInChunk = entityRecord.BufferIndex % storage.GetEntitiesPerChunk();

		uint8_t* chunkData = storage.GetChunk(chunkIndex).GetBuffer();

		return chunkData + archetypeComponents.ComponentArrayOffsets[componentIndex] + indexInChunk * componentSize;
	}

	void* Entities::GetSingletonComponent(ComponentId id) const
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(id));

		auto result = m_Archetypes.GetArchetypesWithComponent(id);
		if (result)
		{
			FLARE_CORE_ERROR("Failed to get singleton component: World doesn't contain any entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
			return nullptr;
		}

		const auto& archetypes = *result;
		
		ArchetypeId archetype = INVALID_ARCHETYPE_ID;
		size_t componentIndex = SIZE_MAX;
		for (const auto& pair : archetypes)
		{
			const EntityStorage& storage = GetEntityStorage(pair.first);
			if (storage.GetEntityCount() != 0)
			{
				if (archetype == INVALID_ARCHETYPE_ID)
				{
					archetype = pair.first;
					componentIndex = pair.second;
				}
				else
				{
					FLARE_CORE_ERROR("Failed to get singleton component: World contains multiple entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
					return nullptr;
				}
			}
		}

		if (archetype == INVALID_ARCHETYPE_ID)
		{
			FLARE_CORE_ERROR("Failed to get singleton component: World doesn't contain any entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
			return nullptr;
		}

		const ArchetypeRecord& record = m_Archetypes[archetype];
		const EntityStorage& storage = GetEntityStorage(archetype);

		if (storage.GetEntityCount() != 1)
		{
			FLARE_CORE_ERROR("Failed to get singleton component: World contains multiple entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
			return nullptr;
		}

		return storage.GetEntityComponentData(0, componentIndex);
	}

	std::optional<Entity> Entities::GetSingletonEntity(const Query& query) const
	{
		FLARE_PROFILE_FUNCTION();
		const auto& archetypes = query.GetMatchingArchetypes();

		ArchetypeId archetype = INVALID_ARCHETYPE_ID;
		size_t componentIndex = SIZE_MAX;
		for (const auto& pair : archetypes)
		{
			const EntityStorage& storage = GetEntityStorage(pair);
			if (storage.GetEntityCount() != 0)
			{
				if (archetype == INVALID_ARCHETYPE_ID)
					archetype = pair;
				else
				{
					FLARE_CORE_ERROR("Failed to get singleton entity: Multiple entities matched the query");
					return {};
				}
			}
		}

		if (archetype == INVALID_ARCHETYPE_ID)
		{
			FLARE_CORE_ERROR("Failed to get singleton entity: Zero entities matched the query");
			return {};
		}

		const ArchetypeRecord& record = m_Archetypes[archetype];
		const EntityStorage& storage = GetEntityStorage(archetype);
		if (storage.GetEntityCount() != 1)
		{
			FLARE_CORE_ERROR("Failed to get singleton entity: Multiple entities matched the query");
			return {};
		}

		return storage.GetEntityId(0);
	}

	EntitiesIterator Entities::begin()
	{
		return EntitiesIterator(*this, 0);
	}

	EntitiesIterator Entities::end()
	{
		return EntitiesIterator(*this, m_EntityRecords.size());
	}

	Span<const ComponentId> Entities::GetEntityComponents(Entity entity)
	{
		auto it = FindEntity(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());

		ArchetypeId archetypeId = m_EntityRecords[it->second].Archetype;
		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(archetypeId);
		return Span(archetypeComponents.ComponentIds, archetypeComponents.ComponentCount);
	}

	bool Entities::HasComponent(Entity entity, ComponentId component) const
	{
		FLARE_PROFILE_FUNCTION();
		auto it = FindEntity(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());

		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(m_EntityRecords[it->second].Archetype);
		return archetypeComponents.TryGetComponentIndex(component) != ArchetypeComponents::INVALID_COMPONENT_INDEX;
	}

	EntityRecord& Entities::operator[](size_t index)
	{
		FLARE_CORE_ASSERT(index < m_EntityRecords.size());
		return m_EntityRecords[index];
	}

	const EntityRecord& Entities::operator[](size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_EntityRecords.size());
		return m_EntityRecords[index];
	}

	void Entities::EnsureValidEntityStorages()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Archetypes.GetArchetypeCount() >= m_EntityStorages.size())
		{
			size_t oldSize = m_EntityStorages.size();
			m_EntityStorages.resize(m_Archetypes.GetArchetypeCount());
			
			for (size_t i = oldSize; i < m_EntityStorages.size(); i++)
			{
				const ArchetypeRecord& archetypeRecord = m_Archetypes[(ArchetypeId)i];
				const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents((ArchetypeId)i);
				FLARE_CORE_ASSERT(archetypeComponents.ComponentCount > 0);

				EntityStorageRequirements storageRequirements{};
				storageRequirements.EntitySize = archetypeRecord.EntitySize;
				storageRequirements.EntityAlignment = archetypeRecord.EntityAlignment;
				m_EntityStorages[i].Initialize(storageRequirements, m_Archetypes, archetypeRecord.Id);
			}
		}
	}

	void Entities::CreateEntity(const ComponentSet& components, EntityCreationResult& result)
	{
		FLARE_PROFILE_FUNCTION();
		const ArchetypeRecord* foundArchetype = m_Archetypes.FindArchetype(components);
		
		if (!foundArchetype)
		{
			ArchetypeId newArchetypeId = m_Archetypes.CreateArchetype(components);
			const ArchetypeRecord& archetype = m_Archetypes[newArchetypeId];

			EntityStorageRequirements storageRequirements{};
			storageRequirements.EntitySize = archetype.EntitySize;
			storageRequirements.EntityAlignment = archetype.EntityAlignment;
			GetEntityStorage(newArchetypeId).Initialize(storageRequirements, m_Archetypes, archetype.Id);

			foundArchetype = &archetype;
		}

		FLARE_CORE_ASSERT(foundArchetype);

		return CreateEntity(*foundArchetype, result);
	}

	void Entities::CreateEntity(const ArchetypeRecord& archetype, EntityCreationResult& result)
	{
		FLARE_PROFILE_FUNCTION();
		size_t registryIndex = m_EntityRecords.size();
		EntityRecord& record = m_EntityRecords.emplace_back();
		record.RegistryIndex = (uint32_t)registryIndex;
		record.Id = m_EntityIndex.CreateId();
		record.Archetype = archetype.Id;

		const ArchetypeRecord& archetypeRecord = m_Archetypes[record.Archetype];
		EntityStorage& storage = GetEntityStorage(record.Archetype);
		record.BufferIndex = storage.AddEntity(record.Id);

		m_EntityToRecord.emplace(record.Id, record.RegistryIndex);

		result.Id = record.Id;
		result.Archetype = record.Archetype;

		if (archetypeRecord.IsUsedInCreatedEntitiesQuery())
		{
			auto it = m_CreatedEntitiesPerArchetype.find(archetypeRecord.Id);
			std::vector<Entity>* entitiesList = nullptr;

			if (it == m_CreatedEntitiesPerArchetype.end())
			{
				auto result = m_CreatedEntitiesPerArchetype.emplace(archetypeRecord.Id, std::vector<Entity>{});
				entitiesList = &result.first->second;
			}
			else
			{
				entitiesList = &it->second;
			}

			entitiesList->push_back(result.Id);
		}
	}

	EntityStorage& Entities::GetDeletedEntityStorage(ArchetypeId archetype)
	{
		FLARE_CORE_ASSERT(m_Archetypes.IsIdValid(archetype));

		auto it = m_DeletedEntitiesStorages.find(archetype);
		if (it == m_DeletedEntitiesStorages.end())
		{
			const ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];
			EntityStorage& storage = m_DeletedEntitiesStorages.insert({ archetype, EntityStorage() }).first->second;

			EntityStorageRequirements storageRequirements{};
			storageRequirements.EntitySize = archetypeRecord.EntitySize;
			storageRequirements.EntityAlignment = archetypeRecord.EntityAlignment;
			storage.Initialize(storageRequirements, m_Archetypes, archetypeRecord.Id);
			return storage;
		}

		return it->second;
	}

	const EntityStorage& Entities::GetDeletedEntityStorage(ArchetypeId archetype) const
	{
		FLARE_CORE_ASSERT(m_Archetypes.IsIdValid(archetype));
		auto it = m_DeletedEntitiesStorages.find(archetype);
		FLARE_CORE_ASSERT(it != m_DeletedEntitiesStorages.end());

		return it->second;
	}

	Span<const Entity> Entities::GetCreatedEntities(ArchetypeId archetype)
	{
		auto it = m_CreatedEntitiesPerArchetype.find(archetype);
		if (it == m_CreatedEntitiesPerArchetype.end())
			return Span<Entity>();

		return Span<Entity>::FromVector(it->second);
	}

	void Entities::ClearQueuedForDeletion()
	{
		FLARE_PROFILE_FUNCTION();

		for (auto& [archetypeId, storage] : m_DeletedEntitiesStorages)
		{
			storage.Release();
		}

		m_DeletedEntitiesStorages.clear();
	}

	void Entities::ClearCreatedEntitiesQueryResult()
	{
		FLARE_PROFILE_FUNCTION();
		for (auto& pair : m_CreatedEntitiesPerArchetype)
		{
			pair.second.clear();
		}
	}

	void Entities::RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex)
	{
		const ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];

		EntityStorage& storage = GetEntityStorage(archetype);
		FLARE_CORE_ASSERT(storage.GetEntityCount() > 0);

		Entity lastEntity = storage.GetEntityId(storage.GetEntityCount() - 1);

		auto it = m_EntityToRecord.find(lastEntity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());

		EntityRecord& lastEntityRecord = m_EntityRecords[it->second];

		storage.RemoveEntity(entityBufferIndex);
		lastEntityRecord.BufferIndex = entityBufferIndex;
	}

	std::unordered_map<Entity, size_t>::iterator Entities::FindEntity(Entity entity)
	{
		auto it = m_EntityToRecord.find(entity);

		if (it == m_EntityToRecord.end())
			return it;

		if (it->first != entity)
			return m_EntityToRecord.end();

		return it;
	}

	std::unordered_map<Entity, size_t>::const_iterator Entities::FindEntity(Entity entity) const
	{
		auto it = m_EntityToRecord.find(entity);

		if (it == m_EntityToRecord.cend())
			return it;

		if (it->first != entity)
			return m_EntityToRecord.cend();

		return it;
	}

	void Entities::ReleaseEntityData()
	{
		FLARE_PROFILE_FUNCTION();

		ClearQueuedForDeletion();

		for (const ArchetypeRecord& archetype : m_Archetypes.GetRecords())
		{
			// HACK: Used to cause a crash when destroying a world owned by prefab editor scene
			//
			//       **The cause of the crash was an empty `m_EntityStorages`**
			// 
			//		 It was empty probably because the prefab window was never used (throughout the lifetime of the application)
			//       and the World stayed empty and thus `EnsureValidEntityStorages()` was never called
			if (archetype.Id >= m_EntityStorages.size())
				continue;

			EntityStorage& storage = m_EntityStorages[archetype.Id];
			storage.Release();
		}
	}

	void Entities::OnArchetypeCreated(ArchetypeId id)
	{
		FLARE_PROFILE_FUNCTION();

		EnsureValidEntityStorages();
	}

	//
	// EntityDataHelper
	//

	void EntityHelper::DefaultConstruct(const Archetypes& archetypes, ArchetypeId archetypeId, const Components& compatibleComponents, void* entityData)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeComponents& archetypeComponents = archetypes.GetArchetypeComponents(archetypeId);
		const ArchetypeRecord& archetype = archetypes[archetypeId];

		FLARE_CORE_ASSERT(HAS_BIT(archetype.CombinedComponentTypeFlags, TypeFlags::DefaultConstructable));

		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& component = compatibleComponents.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
			uint8_t* componentData = (uint8_t*)entityData + archetype.ComponentOffsets[componentIndex];

			component.Initializer->Type.Functions.DefaultConstructor(componentData);
		}
	}

	void EntityHelper::Destroy(const Archetypes& archetypes, ArchetypeId archetypeId, const Components& compatibleComponents, void* entityData)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeComponents& archetypeComponents = archetypes.GetArchetypeComponents(archetypeId);
		const ArchetypeRecord& archetype = archetypes[archetypeId];

		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& component = compatibleComponents.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
			uint8_t* componentData = (uint8_t*)entityData + archetype.ComponentOffsets[componentIndex];

			component.Initializer->Type.Functions.Destructor(componentData);
		}
	}

	void EntityHelper::CopyConstruct(const Archetypes& archetypes, ArchetypeId archetypeId, const Components& compatibleComponents, void* entityData, const void* copySource)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeComponents& archetypeComponents = archetypes.GetArchetypeComponents(archetypeId);
		const ArchetypeRecord& archetype = archetypes[archetypeId];

		if (HAS_BIT(archetype.CombinedComponentTypeFlags, TypeFlags::TriviallyCopyConstructable))
		{
			std::memcpy(entityData, copySource, archetype.EntitySize);
			return;
		}

		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& component = compatibleComponents.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
			size_t componentOffset = archetype.ComponentOffsets[componentIndex];

			component.Initializer->Type.Functions.CopyConstructor((uint8_t*)entityData + componentOffset, (const uint8_t*)copySource + componentOffset);
		}
	}

	void EntityHelper::MoveConstruct(const Archetypes& archetypes, ArchetypeId archetypeId, const Components& compatibleComponents, void* entityData, void* moveSource)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeComponents& archetypeComponents = archetypes.GetArchetypeComponents(archetypeId);
		const ArchetypeRecord& archetype = archetypes[archetypeId];

		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& component = compatibleComponents.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
			size_t componentOffset = archetype.ComponentOffsets[componentIndex];

			component.Initializer->Type.Functions.MoveConstructor((uint8_t*)entityData + componentOffset, (uint8_t*)moveSource + componentOffset);
		}
	}
}