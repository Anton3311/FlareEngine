#include "Entities.h"

#include "FlareECS/Query/EntityView.h"
#include "FlareECS/Query/EntitiesIterator.h"

#include "FlareECS/Query/Query.h"

#include "FlareECS/Entity/ComponentInitializer.h"

#include "FlareECS/EntityStorage/EntityChunksPool.h"

#include <algorithm>

namespace Flare
{
	Entities::Entities(Components& components, QueryCache& queries, Archetypes& archetypes)
		: m_Components(components), m_Queries(queries), m_Archetypes(archetypes)
	{
		EntityChunksPool::Initialize(16);
		ValidateEntityStorages();
	}

	Entities::~Entities()
	{
		for (const ArchetypeRecord& archetype : m_Archetypes.Records)
		{
			EntityStorage& storage = m_EntityStorages[archetype.Id];
			for (size_t entityIndex = 0; entityIndex < storage.GetEntitiesCount(); entityIndex++)
			{
				uint8_t* entityData = storage.GetEntityData(entityIndex);
				for (size_t i = 0; i < archetype.Components.size(); i++)
				{
					m_Components.GetComponentInfo(archetype.Components[i]).Deleter((void*)(entityData + archetype.ComponentOffsets[i]));
				}
			}
		}
	}

	void Entities::Clear()
	{
		for (const ArchetypeRecord& archetype : m_Archetypes.Records)
		{
			if (archetype.Id >= m_EntityStorages.size())
				break;

			EntityStorage& storage = m_EntityStorages[archetype.Id];
			for (size_t entityIndex = 0; entityIndex < storage.GetEntitiesCount(); entityIndex++)
			{
				uint8_t* entityData = storage.GetEntityData(entityIndex);
				for (size_t i = 0; i < archetype.Components.size(); i++)
				{
					m_Components.GetComponentInfo(archetype.Components[i]).Deleter((void*)(entityData + archetype.ComponentOffsets[i]));
				}
			}
		}

		for (const EntityRecord& record : m_EntityRecords)
			m_EntityIndex.AddDeletedId(record.Id);

		m_EntityStorages.clear();
		m_EntityRecords.clear();
		m_EntityToRecord.clear();
		m_TemporaryComponentSet.clear();
	}

	Entity Entities::CreateEntity(const ComponentSet& componentSet, ComponentInitializationStrategy initStrategy)
	{
		FLARE_CORE_ASSERT(componentSet.GetCount() > 0);

		if (m_TemporaryComponentSet.size() < componentSet.GetCount())
			m_TemporaryComponentSet.resize(componentSet.GetCount());

		std::memcpy(m_TemporaryComponentSet.data(), componentSet.GetIds(), sizeof(ComponentId) * componentSet.GetCount());

		std::sort(m_TemporaryComponentSet.data(), m_TemporaryComponentSet.data() + componentSet.GetCount());

		ComponentSet components = ComponentSet(m_TemporaryComponentSet.data(), componentSet.GetCount());

		EntityCreationResult result;
		CreateEntity(components, result);
		InitializeEntity(result, initStrategy);

		return result.Id;
	}

	Entity Entities::CreateEntity(const std::pair<ComponentId, const void*>* components, size_t count)
	{
		if (m_TemporaryComponentSet.size() < count)
			m_TemporaryComponentSet.resize(count);

		for (size_t i = 0; i < count; i++)
			m_TemporaryComponentSet[i] = components[i].first;

		EntityCreationResult result;
		CreateEntity(ComponentSet(m_TemporaryComponentSet.data(), count), result);

		const ArchetypeRecord& archetypeRecord = m_Archetypes[result.Archetype];
		for (size_t i = 0; i < count; i++)
		{
			if (components[i].second == nullptr)
				continue;

			size_t componentSize = 0;
			if (i == count - 1)
				componentSize = GetEntityStorage(result.Archetype).GetEntitySize() - archetypeRecord.ComponentOffsets[count - 1];
			else
				componentSize = archetypeRecord.ComponentOffsets[i + 1] - archetypeRecord.ComponentOffsets[i];

			std::memcpy(result.Data + archetypeRecord.ComponentOffsets[i], components[i].second, componentSize);
		}

		return result.Id;
	}

	Entity Entities::CreateEntityFromArchetype(ArchetypeId archetype, ComponentInitializationStrategy initStrategy)
	{
		FLARE_CORE_ASSERT((size_t)archetype < m_Archetypes.Records.size());

		size_t registryIndex = m_EntityRecords.size();
		EntityRecord& record = m_EntityRecords.emplace_back();

		record.RegistryIndex = (uint32_t)registryIndex;
		record.Id = m_EntityIndex.CreateId();
		record.Archetype = archetype;

		const ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];
		EntityStorage& storage = GetEntityStorage(archetype);

		record.BufferIndex = storage.AddEntity(record.RegistryIndex);

		uint8_t* entityData = storage.GetEntityData(record.BufferIndex);
		switch (initStrategy)
		{
		case ComponentInitializationStrategy::Zero:
		{
			// Intialize entity data to 0
			std::memset(entityData, 0, storage.GetEntitySize());
			break;
		}
		case ComponentInitializationStrategy::DefaultConstructor:
		{
			for (size_t i = 0; i < archetypeRecord.Components.size(); i++)
			{
				const ComponentInfo& info = m_Components.GetComponentInfo(archetypeRecord.Components[i]);
				uint8_t* componentData = entityData + archetypeRecord.ComponentOffsets[i];

				if (info.Initializer)
					info.Initializer->Type.DefaultConstructor(componentData);
				else
					std::memset(componentData, 0, info.Size);
			}

			break;
		}
		}

		m_EntityToRecord.emplace(record.Id, record.RegistryIndex);
		return record.Id;
	}

	void Entities::DeleteEntity(Entity entity)
	{
		auto recordIterator = FindEntity(entity);
		if (recordIterator == m_EntityToRecord.end())
			return;

		EntityRecord& record = m_EntityRecords[recordIterator->second];
		EntityRecord& lastEntityRecord = m_EntityRecords.back();

		EntityStorage& storage = GetEntityStorage(record.Archetype);

		uint32_t lastEntityInBuffer = storage.GetEntityIndices().back();
		if (lastEntityInBuffer != record.RegistryIndex)
			m_EntityRecords[lastEntityInBuffer].BufferIndex = record.BufferIndex;

		storage.RemoveEntityData(record.BufferIndex);

		m_EntityIndex.AddDeletedId(record.Id);
		m_EntityToRecord.erase(record.Id);

		lastEntityRecord.RegistryIndex = record.RegistryIndex;
		record = lastEntityRecord;

		if (lastEntityRecord.Id != entity)
		{
			GetEntityStorage(record.Archetype).UpdateEntityRegistryIndex(record.BufferIndex, record.RegistryIndex);
			m_EntityToRecord[record.Id] = record.RegistryIndex;
		}

		m_EntityRecords.erase(m_EntityRecords.end() - 1);
	}

	bool Entities::AddEntityComponent(Entity entity, ComponentId componentId, const void* componentData, ComponentInitializationStrategy initStrategy)
	{
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(componentId), "Invalid component id");

		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(componentId);

		auto recordIterator = FindEntity(entity);
		if (recordIterator == m_EntityToRecord.end())
			return {};

		EntityRecord& entityRecord = m_EntityRecords[recordIterator->second];
		ArchetypeRecord& archetype = m_Archetypes.Records[entityRecord.Archetype];

		// Can only have one instance of a component
		if (m_Archetypes.GetArchetypeComponentIndex(entityRecord.Archetype, componentId).has_value())
			return false;

		size_t oldComponentCount = archetype.Components.size();
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;

		size_t insertedComponentIndex = SIZE_MAX;

		auto edgeIterator = archetype.Edges.find(componentId);
		if (edgeIterator != archetype.Edges.end())
		{
			newArchetypeId = edgeIterator->second.Add;
			
			std::optional<size_t> index = m_Archetypes.GetArchetypeComponentIndex(newArchetypeId, componentId);
			if (index.has_value())
				insertedComponentIndex = index.value();
			else
			{
				FLARE_CORE_ASSERT(false, "Archetype doesn't have a component because archetype graph has invalid edge connection");
				return false;
			}
		}
		else
		{
			std::vector<ComponentId> newComponents(oldComponentCount + 1);

			std::memcpy(newComponents.data(), archetype.Components.data(), oldComponentCount * sizeof(componentId));
			newComponents[oldComponentCount] = componentId;

			insertedComponentIndex = oldComponentCount;
			for (size_t i = insertedComponentIndex; i > 0; i--)
			{
				if (newComponents[i - 1] > newComponents[i])
				{
					std::swap(newComponents[i - 1], newComponents[i]);
					insertedComponentIndex = i - 1;
					break;
				}
			}

			bool shouldNotifyQueryCache = false;
			auto it = m_Archetypes.ComponentSetToArchetype.find(ComponentSet(newComponents));

			if (it != m_Archetypes.ComponentSetToArchetype.end())
			{
				newArchetypeId = it->second;
			}
			else
			{
				newArchetypeId = CreateArchetype();
				EntityStorage& storage = GetEntityStorage(newArchetypeId);

				ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];
				newArchetype.Components = std::move(newComponents);
				newArchetype.ComponentOffsets.resize(newArchetype.Components.size());

				size_t entitySize = 0;
				for (size_t i = 0; i < newArchetype.Components.size(); i++)
				{
					newArchetype.ComponentOffsets[i] = entitySize;
					entitySize += m_Components.GetComponentInfo(newArchetype.Components[i]).Size;
				}

				storage.SetEntitySize(entitySize);
				m_Archetypes.ComponentSetToArchetype.emplace(ComponentSet(newArchetype.Components), newArchetypeId);

				shouldNotifyQueryCache = true;
			}

			m_Archetypes.Records[entityRecord.Archetype].Edges.emplace(componentId, ArchetypeEdge{newArchetypeId, INVALID_ARCHETYPE_ID});
			m_Archetypes.Records[newArchetypeId].Edges.emplace(componentId, ArchetypeEdge{INVALID_ARCHETYPE_ID, entityRecord.Archetype});

			ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];
			for (size_t i = 0; i < newArchetype.Components.size(); i++)
				m_Archetypes.ComponentToArchetype[newArchetype.Components[i]].emplace(newArchetypeId, i);

			if (shouldNotifyQueryCache)
				m_Queries.OnArchetypeCreated(newArchetypeId);
		}

		FLARE_CORE_ASSERT(insertedComponentIndex != SIZE_MAX);

		ArchetypeRecord& oldArchetype = m_Archetypes.Records[entityRecord.Archetype];
		ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];

		EntityStorage& oldStorage = GetEntityStorage(oldArchetype.Id);
		EntityStorage& newStorage = GetEntityStorage(newArchetypeId);

		size_t newEntityIndex = newStorage.AddEntity(entityRecord.RegistryIndex);
		uint8_t* newEntityData = newStorage.GetEntityData(newEntityIndex);
		const uint8_t* oldEntityData = oldStorage.GetEntityData(entityRecord.BufferIndex);

		size_t sizeBefore = oldStorage.GetEntitySize();
		if (insertedComponentIndex < oldArchetype.Components.size())
			sizeBefore = oldArchetype.ComponentOffsets[insertedComponentIndex];

		size_t sizeAfter = oldStorage.GetEntitySize() - sizeBefore;

		std::memcpy(newEntityData, oldEntityData, sizeBefore);
		std::memcpy(newEntityData + sizeBefore + componentInfo.Size, oldEntityData + sizeBefore, sizeAfter);

		if (componentData == nullptr)
		{
			uint8_t* componentData = newEntityData + sizeBefore;
			if (initStrategy == ComponentInitializationStrategy::Zero || !componentInfo.Initializer)
				std::memset(componentData, 0, componentInfo.Size);
			else
				componentInfo.Initializer->Type.DefaultConstructor(componentData);
		}
		else
			std::memcpy(newEntityData + sizeBefore, componentData, componentInfo.Size);

		RemoveEntityData(entityRecord.Archetype, entityRecord.BufferIndex);

		entityRecord.Archetype = newArchetypeId;
		entityRecord.BufferIndex = newEntityIndex;

		return true;
	}

	bool Entities::RemoveEntityComponent(Entity entity, ComponentId componentId)
	{
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(componentId), "Invalid component id");

		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(componentId);

		auto recordIterator = FindEntity(entity);
		if (recordIterator == m_EntityToRecord.end())
			return false;

		EntityRecord& entityRecord = m_EntityRecords[recordIterator->second];
		ArchetypeRecord& archetype = m_Archetypes.Records[entityRecord.Archetype];

		size_t removedComponentIndex = SIZE_MAX;
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;
		
		{
			std::optional<size_t> componentIndex = m_Archetypes.GetArchetypeComponentIndex(entityRecord.Archetype, componentId);
			if (componentIndex.has_value())
				removedComponentIndex = componentIndex.value();
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
			size_t oldComponentCount = archetype.Components.size();
			std::vector<ComponentId> newComponents(oldComponentCount - 1);

			for (size_t insertIndex = 0, i = 0; i < oldComponentCount; i++)
			{
				if (archetype.Components[i] == componentId)
					continue;
				else
				{
					newComponents[insertIndex] = archetype.Components[i];
					insertIndex++;
				}
			}

			auto it = m_Archetypes.ComponentSetToArchetype.find(ComponentSet(newComponents));
			if (it != m_Archetypes.ComponentSetToArchetype.end())
			{
				newArchetypeId = it->second;
			}
			else
			{
				newArchetypeId = CreateArchetype();
				EntityStorage& storage = GetEntityStorage(newArchetypeId);

				ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];
				newArchetype.Components = std::move(newComponents);
				newArchetype.ComponentOffsets.resize(newArchetype.Components.size());

				size_t entitySize = 0;
				for (size_t i = 0; i < newArchetype.Components.size(); i++)
				{
					newArchetype.ComponentOffsets[i] = entitySize;
					entitySize += m_Components.GetComponentInfo(newArchetype.Components[i]).Size;
				}

				storage.SetEntitySize(entitySize);
				m_Archetypes.ComponentSetToArchetype.emplace(ComponentSet(newArchetype.Components), newArchetypeId);
			}

			m_Archetypes.Records[entityRecord.Archetype].Edges.emplace(componentId, ArchetypeEdge{ INVALID_ARCHETYPE_ID, newArchetypeId});
			m_Archetypes.Records[newArchetypeId].Edges.emplace(componentId, ArchetypeEdge{ entityRecord.Archetype, INVALID_ARCHETYPE_ID });

			ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];
			for (size_t i = 0; i < newArchetype.Components.size(); i++)
				m_Archetypes.ComponentToArchetype[newArchetype.Components[i]].emplace(newArchetypeId, i);
		}

		ArchetypeRecord& oldArchetype = m_Archetypes.Records[entityRecord.Archetype];
		ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];

		EntityStorage& oldStorage = GetEntityStorage(oldArchetype.Id);
		EntityStorage& newStorage = GetEntityStorage(newArchetypeId);

		size_t sizeBefore = oldArchetype.ComponentOffsets[removedComponentIndex];
		size_t sizeAfter = oldStorage.GetEntitySize() - (sizeBefore + componentInfo.Size);

		size_t newEntityIndex = newStorage.AddEntity(entityRecord.RegistryIndex);

		uint8_t* newEntityData = newStorage.GetEntityData(newEntityIndex);
		uint8_t* oldEntityData = oldStorage.GetEntityData(entityRecord.BufferIndex);

		componentInfo.Deleter(oldEntityData + sizeBefore);

		std::memcpy(newEntityData, oldEntityData, sizeBefore);
		std::memcpy(newEntityData + sizeBefore, oldEntityData + sizeBefore + componentInfo.Size, sizeAfter);

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

	std::optional<uint8_t*> Entities::GetEntityData(Entity entity)
	{
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		EntityRecord& record = m_EntityRecords[it->second];
		return GetEntityStorage(record.Archetype).GetEntityData(record.BufferIndex);
	}

	std::optional<const uint8_t*> Entities::GetEntityData(Entity entity) const
	{
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		const EntityRecord& record = m_EntityRecords[it->second];
		const EntityStorage& storage = GetEntityStorage(record.Archetype);
		return storage.GetEntityData(record.BufferIndex);
	}

	std::optional<size_t> Entities::GetEntityDataSize(Entity entity) const
	{
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		const EntityRecord& record = m_EntityRecords[it->second];
		return GetEntityStorage(record.Archetype).GetEntitySize();
	}

	std::optional<void*> Entities::GetEntityComponent(Entity entity, ComponentId component)
	{
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		const EntityRecord& entityRecord = m_EntityRecords[it->second];
		const ArchetypeRecord& archetype = m_Archetypes.Records[entityRecord.Archetype];
		const EntityStorage& storage = GetEntityStorage(entityRecord.Archetype);

		std::optional<size_t> componentIndex = m_Archetypes.GetArchetypeComponentIndex(entityRecord.Archetype, component);
		if (!componentIndex.has_value())
			return {};

		uint8_t* entityData = storage.GetEntityData(entityRecord.BufferIndex);
		return entityData + archetype.ComponentOffsets[componentIndex.value()];
	}

	std::optional<const void*> Entities::GetEntityComponent(Entity entity, ComponentId component) const
	{
		auto it = FindEntity(entity);
		if (it == m_EntityToRecord.end())
			return {};

		const EntityRecord& entityRecord = m_EntityRecords[it->second];
		const ArchetypeRecord& archetype = m_Archetypes.Records[entityRecord.Archetype];
		const EntityStorage& storage = GetEntityStorage(entityRecord.Archetype);

		std::optional<size_t> componentIndex = m_Archetypes.GetArchetypeComponentIndex(entityRecord.Archetype, component);
		if (!componentIndex.has_value())
			return {};

		const uint8_t* entityData = storage.GetEntityData(entityRecord.BufferIndex);
		return entityData + archetype.ComponentOffsets[componentIndex.value()];
	}

	std::optional<void*> Entities::GetSingletonComponent(ComponentId id) const
	{
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(id));

		auto it = m_Archetypes.ComponentToArchetype.find(id);
		if (it == m_Archetypes.ComponentToArchetype.end())
		{
			FLARE_CORE_ERROR("Failed to get singleton component: World doesn't contain any entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
			return nullptr;
		}

		const auto& archetypes = it->second;
		
		ArchetypeId archetype = INVALID_ARCHETYPE_ID;
		size_t componentIndex = SIZE_MAX;
		for (const auto& pair : archetypes)
		{
			const EntityStorage& storage = GetEntityStorage(pair.first);
			if (storage.GetEntitiesCount() != 0)
			{
				if (archetype == INVALID_ARCHETYPE_ID)
				{
					archetype = pair.first;
					componentIndex = pair.second;
				}
				else
				{
					FLARE_CORE_ERROR("Failed to get singleton component: World contains multiple entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
					return {};
				}
			}
		}

		if (archetype == INVALID_ARCHETYPE_ID)
		{
			FLARE_CORE_ERROR("Failed to get singleton component: World doesn't contain any entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
			return {};
		}

		const ArchetypeRecord& record = m_Archetypes[archetype];
		const EntityStorage& storage = GetEntityStorage(archetype);

		if (storage.GetEntitiesCount() != 1)
		{
			FLARE_CORE_ERROR("Failed to get singleton component: World contains multiple entities with component '{0}'", m_Components.GetComponentInfo(id).Name);
			return {};
		}

		uint8_t* entityData = storage.GetEntityData(0);
		return entityData + record.ComponentOffsets[componentIndex];
	}

	std::optional<Entity> Entities::GetSingletonEntity(const Query& query) const
	{
		const auto& archetypes = query.GetMatchedArchetypes();

		ArchetypeId archetype = INVALID_ARCHETYPE_ID;
		size_t componentIndex = SIZE_MAX;
		for (const auto& pair : archetypes)
		{
			const EntityStorage& storage = GetEntityStorage(pair);
			if (storage.GetEntitiesCount() != 0)
			{
				if (archetype == INVALID_ARCHETYPE_ID)
					archetype = pair;
				else
				{
					FLARE_CORE_ERROR("Failed to get singleton entity: Multiple entities matched the query");
					return Entity();
				}
			}
		}

		if (archetype == INVALID_ARCHETYPE_ID)
		{
			FLARE_CORE_ERROR("Failed to get singleton entity: Zero entities matched the query");
			return Entity();
		}

		const ArchetypeRecord& record = m_Archetypes[archetype];
		const EntityStorage& storage = GetEntityStorage(archetype);
		if (storage.GetEntitiesCount() != 1)
		{
			FLARE_CORE_ERROR("Failed to get singleton entity: Multiple entities matched the query");
			return Entity();
		}

		return m_EntityRecords[storage.GetEntityIndices()[0]].Id;
	}

	EntitiesIterator Entities::begin()
	{
		return EntitiesIterator(*this, 0);
	}

	EntitiesIterator Entities::end()
	{
		return EntitiesIterator(*this, m_EntityRecords.size());
	}

	const std::vector<ComponentId>& Entities::GetEntityComponents(Entity entity)
	{
		auto it = FindEntity(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());
		return m_Archetypes.Records[m_EntityRecords[it->second].Archetype].Components;
	}

	bool Entities::HasComponent(Entity entity, ComponentId component) const
	{
		auto it = FindEntity(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());
		return m_Archetypes.GetArchetypeComponentIndex(m_EntityRecords[it->second].Archetype, component).has_value();
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

	void Entities::CreateEntity(const ComponentSet& components, EntityCreationResult& result)
	{
		size_t registryIndex = m_EntityRecords.size();
		EntityRecord& record = m_EntityRecords.emplace_back();

		record.RegistryIndex = (uint32_t)registryIndex;
		record.Id = m_EntityIndex.CreateId();

		auto it = m_Archetypes.ComponentSetToArchetype.find(components);
		if (it != m_Archetypes.ComponentSetToArchetype.end())
			record.Archetype = it->second;
		else
		{
			ArchetypeId newArchetypeId = CreateArchetype();
			ArchetypeRecord& newArchetype = m_Archetypes.Records[newArchetypeId];
			record.Archetype = newArchetype.Id;

			newArchetype.ComponentOffsets.resize(components.GetCount());
			newArchetype.Components.resize(components.GetCount());

			std::memcpy(newArchetype.Components.data(), components.GetIds(), components.GetCount() * sizeof(size_t));

			size_t entitySize = 0;
			for (size_t i = 0; i < components.GetCount(); i++)
			{
				newArchetype.ComponentOffsets[i] = entitySize;
				entitySize += m_Components.GetComponentInfo(components[i]).Size;
			}

			GetEntityStorage(newArchetypeId).SetEntitySize(entitySize);

			m_Archetypes.ComponentSetToArchetype.emplace(ComponentSet(newArchetype.Components), newArchetype.Id);

			for (size_t i = 0; i < newArchetype.Components.size(); i++)
				m_Archetypes.ComponentToArchetype[newArchetype.Components[i]].emplace(newArchetypeId, i);

			m_Queries.OnArchetypeCreated(newArchetypeId);
		}

		ArchetypeRecord& archetypeRecord = m_Archetypes.Records[record.Archetype];
		EntityStorage& storage = GetEntityStorage(record.Archetype);
		record.BufferIndex = storage.AddEntity(record.RegistryIndex);

		m_EntityToRecord.emplace(record.Id, record.RegistryIndex);

		result.Id = record.Id;
		result.Archetype = record.Archetype;
		result.Data = storage.GetEntityData(record.BufferIndex);
	}

	void Entities::InitializeEntity(const EntityCreationResult& entityResult, ComponentInitializationStrategy initStrategy)
	{
		switch (initStrategy)
		{
		case ComponentInitializationStrategy::Zero:
		{
			// Intialize entity data to 0
			std::memset(entityResult.Data, 0, GetEntityStorage(entityResult.Archetype).GetEntitySize());
			break;
		}
		case ComponentInitializationStrategy::DefaultConstructor:
		{
			const ArchetypeRecord& archetypeRecord = m_Archetypes[entityResult.Archetype];
			for (size_t i = 0; i < archetypeRecord.Components.size(); i++)
			{
				const ComponentInfo& info = m_Components.GetComponentInfo(archetypeRecord.Components[i]);
				uint8_t* componentData = entityResult.Data + archetypeRecord.ComponentOffsets[i];

				if (info.Initializer)
					info.Initializer->Type.DefaultConstructor(componentData);
				else
					std::memset(componentData, 0, info.Size);
			}

			break;
		}
		}
	}

	ArchetypeId Entities::CreateArchetype()
	{
		ArchetypeId id = m_Archetypes.Records.size();
		ArchetypeRecord& record = m_Archetypes.Records.emplace_back();
		record.Id = id;

		m_EntityStorages.emplace_back();
		return id;
	}

	EntityStorage& Entities::GetEntityStorage(ArchetypeId archetype)
	{
		FLARE_CORE_ASSERT(m_Archetypes.IsIdValid(archetype));
		ValidateEntityStorages();
		return m_EntityStorages[archetype];
	}

	const EntityStorage& Entities::GetEntityStorage(ArchetypeId archetype) const
	{
		FLARE_CORE_ASSERT(m_Archetypes.IsIdValid(archetype));
		return m_EntityStorages[archetype];
	}

	void Entities::RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex)
	{
		ArchetypeRecord& archetypeRecord = m_Archetypes.Records[archetype];

		EntityStorage& storage = GetEntityStorage(archetype);
		EntityRecord& lastEntityRecord = m_EntityRecords[storage.GetEntityIndices().back()];

		storage.RemoveEntityData(entityBufferIndex);
		lastEntityRecord.BufferIndex = entityBufferIndex;
	}

	void Entities::ValidateEntityStorages()
	{
		size_t oldSize = m_EntityStorages.size();
		if (m_EntityStorages.size() < m_Archetypes.Records.size())
		{
			m_EntityStorages.resize(m_Archetypes.Records.size());

			for (size_t i = oldSize; i < m_EntityStorages.size(); i++)
			{
				const ArchetypeRecord& record = m_Archetypes.Records[i];

				size_t entitySize = record.ComponentOffsets.back() + m_Components.GetComponentInfo(record.Components.back()).Size;
				m_EntityStorages[i].SetEntitySize(entitySize);
			}
		}
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
}