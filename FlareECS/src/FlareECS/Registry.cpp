#include "Registry.h"

#include "FlareECS/Query/EntityView.h"
#include "FlareECS/Query/EntityRegistryIterator.h"
#include "FlareECS/Query/EntityArchetypesView.h"

#include <algorithm>

namespace Flare
{
	Entity Registry::CreateEntity(ComponentSet& components)
	{
		FLARE_CORE_ASSERT(components.GetCount() > 0);

		std::sort(components.GetIds(), components.GetIds() + components.GetCount());

		size_t registryIndex = m_EntityRecords.size();
		EntityRecord& record = m_EntityRecords.emplace_back();

		record.RegistryIndex = registryIndex;
		record.Id = Entity(record.RegistryIndex);

		auto it = m_ComponentSetToArchetype.find(components);
		if (it != m_ComponentSetToArchetype.end())
			record.Archetype = it->second;
		else
		{
			ArchetypeId newArchetypeId = CreateArchetype();
			ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
			record.Archetype = newArchetype.Data.Id;

			newArchetype.Data.ComponentOffsets.resize(components.GetCount());
			newArchetype.Data.Components.resize(components.GetCount());

			std::memcpy(newArchetype.Data.Components.data(), components.GetIds(), components.GetCount() * sizeof(size_t));

			size_t entitySize = 0;
			for (size_t i = 0; i < components.GetCount(); i++)
			{
				newArchetype.Data.ComponentOffsets[i] = entitySize;
				entitySize += GetComponentInfo(components[i]).Size;
			}

			newArchetype.Storage.SetEntitySize(entitySize);

			m_ComponentSetToArchetype.emplace(ComponentSet(newArchetype.Data.Components), newArchetype.Data.Id);

			for (size_t i = 0; i < newArchetype.Data.Components.size(); i++)
				m_ComponentToArchetype[newArchetype.Data.Components[i]].emplace(newArchetypeId, i);

			m_QueryCache.OnArchetypeCreated(newArchetypeId);
		}

		ArchetypeRecord& archetypeRecord = m_Archetypes[record.Archetype];
		record.BufferIndex = archetypeRecord.Storage.AddEntity(record.RegistryIndex);

		m_EntityToRecord.emplace(record.Id, record.RegistryIndex);
		return record.Id;
	}

	bool Registry::AddEntityComponent(Entity entity, ComponentId componentId, const void* componentData)
	{
		if (componentId >= m_RegisteredComponents.size())
		{
			FLARE_CORE_WARN("Provided component was not registered");
			return false;
		}

		auto it = m_EntityToRecord.find(entity);
		if (it == m_EntityToRecord.end())
			return {};

		EntityRecord& entityRecord = m_EntityRecords[it->second];
		ArchetypeRecord& archetype = m_Archetypes[entityRecord.Archetype];

		// Can only have one instance of a component
		if (GetArchetypeComponentIndex(entityRecord.Archetype, componentId).has_value())
			return false;

		size_t oldComponentCount = archetype.Data.Components.size();
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;

		size_t insertedComponentIndex = SIZE_MAX;

		auto edgeIterator = archetype.Data.Edges.find(componentId);
		if (edgeIterator != archetype.Data.Edges.end())
		{
			newArchetypeId = edgeIterator->second.Add;
			
			std::optional<size_t> index = GetArchetypeComponentIndex(newArchetypeId, componentId);
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

			std::memcpy(newComponents.data(), archetype.Data.Components.data(), oldComponentCount * sizeof(componentId));
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
			auto it = m_ComponentSetToArchetype.find(ComponentSet(newComponents));
			if (it != m_ComponentSetToArchetype.end())
			{
				newArchetypeId = it->second;
			}
			else
			{
				newArchetypeId = CreateArchetype();

				ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
				newArchetype.Data.Components = std::move(newComponents);
				newArchetype.Data.ComponentOffsets.resize(newArchetype.Data.Components.size());

				size_t entitySize = 0;
				for (size_t i = 0; i < newArchetype.Data.Components.size(); i++)
				{
					newArchetype.Data.ComponentOffsets[i] = entitySize;
					entitySize += m_RegisteredComponents[newArchetype.Data.Components[i]].Size;
				}

				newArchetype.Storage.SetEntitySize(entitySize);
				m_ComponentSetToArchetype.emplace(ComponentSet(newArchetype.Data.Components), newArchetypeId);

				shouldNotifyQueryCache = true;
			}

			m_Archetypes[entityRecord.Archetype].Data.Edges.emplace(componentId, ArchetypeEdge{newArchetypeId, INVALID_ARCHETYPE_ID});
			m_Archetypes[newArchetypeId].Data.Edges.emplace(componentId, ArchetypeEdge{INVALID_ARCHETYPE_ID, entityRecord.Archetype});

			ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
			for (size_t i = 0; i < newArchetype.Data.Components.size(); i++)
				m_ComponentToArchetype[newArchetype.Data.Components[i]].emplace(newArchetypeId, i);

			if (shouldNotifyQueryCache)
				m_QueryCache.OnArchetypeCreated(newArchetypeId);
		}

		FLARE_CORE_ASSERT(insertedComponentIndex != SIZE_MAX);

		ArchetypeRecord& oldArchetype = m_Archetypes[entityRecord.Archetype];
		ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];

		size_t newEntityIndex = newArchetype.Storage.AddEntity(entityRecord.RegistryIndex);
		uint8_t* newEntityData = newArchetype.Storage.GetEntityData(newEntityIndex);
		const uint8_t* oldEntityData = oldArchetype.Storage.GetEntityData(entityRecord.BufferIndex);

		size_t componentSize = m_RegisteredComponents[componentId].Size;
		size_t sizeBefore = oldArchetype.Storage.GetEntitySize();
		if (insertedComponentIndex < oldArchetype.Data.Components.size())
			sizeBefore = oldArchetype.Data.ComponentOffsets[insertedComponentIndex];

		size_t sizeAfter = oldArchetype.Storage.GetEntitySize() - sizeBefore;

		std::memcpy(newEntityData, oldEntityData, sizeBefore);
		std::memcpy(newEntityData + sizeBefore, componentData, componentSize);
		std::memcpy(newEntityData + sizeBefore + componentSize, oldEntityData + sizeBefore, sizeAfter);

		RemoveEntityData(entityRecord.Archetype, entityRecord.BufferIndex);

		entityRecord.Archetype = newArchetypeId;
		entityRecord.BufferIndex = newEntityIndex;
	}

	bool Registry::RemoveEntityComponent(Entity entity, ComponentId componentId)
	{
		if (componentId >= m_RegisteredComponents.size())
		{
			FLARE_CORE_WARN("Provided component was not registered");
			return false;
		}

		auto it = m_EntityToRecord.find(entity);
		if (it == m_EntityToRecord.end())
			return {};

		EntityRecord& entityRecord = m_EntityRecords[it->second];
		ArchetypeRecord& archetype = m_Archetypes[entityRecord.Archetype];

		size_t removedComponentIndex = SIZE_MAX;
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;
		
		{
			std::optional<size_t> componentIndex = GetArchetypeComponentIndex(entityRecord.Archetype, componentId);
			if (componentIndex.has_value())
				removedComponentIndex = componentIndex.value();
			else
				return false;
		}

		auto edgeIterator = archetype.Data.Edges.find(componentId);
		if (edgeIterator != archetype.Data.Edges.end())
		{
			newArchetypeId = edgeIterator->second.Remove;
		}
		else
		{
			size_t oldComponentCount = archetype.Data.Components.size();
			std::vector<ComponentId> newComponents(oldComponentCount - 1);

			for (size_t insertIndex = 0, i = 0; i < oldComponentCount; i++)
			{
				if (archetype.Data.Components[i] == componentId)
					continue;
				else
				{
					newComponents[insertIndex] = archetype.Data.Components[i];
					insertIndex++;
				}
			}

			auto it = m_ComponentSetToArchetype.find(ComponentSet(newComponents));
			if (it != m_ComponentSetToArchetype.end())
			{
				newArchetypeId = it->second;
			}
			else
			{
				newArchetypeId = CreateArchetype();

				ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
				newArchetype.Data.Components = std::move(newComponents);
				newArchetype.Data.ComponentOffsets.resize(newArchetype.Data.Components.size());

				size_t entitySize = 0;
				for (size_t i = 0; i < newArchetype.Data.Components.size(); i++)
				{
					newArchetype.Data.ComponentOffsets[i] = entitySize;
					entitySize += m_RegisteredComponents[newArchetype.Data.Components[i]].Size;
				}

				newArchetype.Storage.SetEntitySize(entitySize);
				m_ComponentSetToArchetype.emplace(ComponentSet(newArchetype.Data.Components), newArchetypeId);
			}

			m_Archetypes[entityRecord.Archetype].Data.Edges.emplace(componentId, ArchetypeEdge{ INVALID_ARCHETYPE_ID, newArchetypeId});
			m_Archetypes[newArchetypeId].Data.Edges.emplace(componentId, ArchetypeEdge{ entityRecord.Archetype, INVALID_ARCHETYPE_ID });

			ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];
			for (size_t i = 0; i < newArchetype.Data.Components.size(); i++)
				m_ComponentToArchetype[newArchetype.Data.Components[i]].emplace(newArchetypeId, i);
		}

		ArchetypeRecord& oldArchetype = m_Archetypes[entityRecord.Archetype];
		ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];

		size_t sizeBefore = oldArchetype.Data.ComponentOffsets[removedComponentIndex];
		size_t componentSize = m_RegisteredComponents[componentId].Size;
		size_t sizeAfter = oldArchetype.Storage.GetEntitySize() - (sizeBefore + componentSize);

		size_t newEntityIndex = newArchetype.Storage.AddEntity(entityRecord.RegistryIndex);

		uint8_t* newEntityData = newArchetype.Storage.GetEntityData(newEntityIndex);
		const uint8_t* oldEntityData = oldArchetype.Storage.GetEntityData(entityRecord.BufferIndex);

		std::memcpy(newEntityData, oldEntityData, sizeBefore);
		std::memcpy(newEntityData + sizeBefore, oldEntityData + sizeBefore + componentSize, sizeAfter);

		RemoveEntityData(entityRecord.Archetype, entityRecord.BufferIndex);

		entityRecord.Archetype = newArchetypeId;
		entityRecord.BufferIndex = newEntityIndex;
	}

	std::optional<void*> Registry::GetEntityComponent(Entity entity, ComponentId component)
	{
		auto it = m_EntityToRecord.find(entity);
		if (it == m_EntityToRecord.end())
			return {};

		EntityRecord& entityRecord = m_EntityRecords[it->second];
		ArchetypeRecord& archetype = m_Archetypes[entityRecord.Archetype];

		std::optional<size_t> componentIndex = GetArchetypeComponentIndex(entityRecord.Archetype, component);
		if (!componentIndex.has_value())
			return {};

		uint8_t* entityData = archetype.Storage.GetEntityData(entityRecord.BufferIndex);
		return entityData + archetype.Data.ComponentOffsets[componentIndex.value()];
	}

	ComponentId Registry::RegisterComponent(std::string_view name, size_t size)
	{
		size_t id = m_RegisteredComponents.size();
		ComponentInfo& info = m_RegisteredComponents.emplace_back();

		info.Id = id;
		info.Name = name;
		info.Size = size;

		return id;
	}

	inline const ComponentInfo& Registry::GetComponentInfo(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_RegisteredComponents.size());
		return m_RegisteredComponents[index];
	}

	EntityRegistryIterator Registry::begin()
	{
		return EntityRegistryIterator(*this, 0);
	}

	EntityRegistryIterator Registry::end()
	{
		return EntityRegistryIterator(*this, m_EntityRecords.size());
	}

	QueryId Registry::CreateQuery(const ComponentSet& components)
	{
		return m_QueryCache.AddQuery(components);
	}

	EntityView Registry::QueryArchetype(ComponentSet components)
	{
		std::sort(components.GetIds(), components.GetIds() + components.GetCount());
		auto it = m_ComponentSetToArchetype.find(components);

		FLARE_CORE_ASSERT(it != m_ComponentSetToArchetype.end());

		size_t archetypeId = it->second;
		return EntityView(*this, archetypeId);
	}

	EntityArchetypesView Registry::ExecuteQuery(QueryId query)
	{
		const QueryData& data = m_QueryCache[query];
		return EntityArchetypesView(*this, data.MatchedArchetypes);
	}

	const ComponentSet& Registry::GetEntityComponents(Entity entity)
	{
		auto it = m_EntityToRecord.find(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());
		return ComponentSet(m_Archetypes[m_EntityRecords[it->second].Archetype].Data.Components);
	}

	bool Registry::HasComponent(Entity entity, ComponentId component)
	{
		auto it = m_EntityToRecord.find(entity);
		FLARE_CORE_ASSERT(it != m_EntityToRecord.end());
		return GetArchetypeComponentIndex(m_EntityRecords[it->second].Archetype, component).has_value();
	}

	inline EntityRecord& Registry::operator[](size_t index)
	{
		FLARE_CORE_ASSERT(index < m_EntityRecords.size());
		return m_EntityRecords[index];
	}

	inline const EntityRecord& Registry::operator[](size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_EntityRecords.size());
		return m_EntityRecords[index];
	}

	ArchetypeId Registry::CreateArchetype()
	{
		ArchetypeId id = m_Archetypes.size();
		ArchetypeRecord& record = m_Archetypes.emplace_back();
		record.Data.Id = id;
		
		return id;
	}

	std::optional<size_t> Registry::GetArchetypeComponentIndex(ArchetypeId archetype, ComponentId component)
	{
		auto it = m_ComponentToArchetype.find(component);
		if (it != m_ComponentToArchetype.end())
		{
			auto it2 = it->second.find(archetype);
			if (it2 != it->second.end())
			{
				return { it2->second };
			}
			else
				return {};
		}
		else
			return {};
	}

	void Registry::RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex)
	{
		ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];
		EntityRecord& lastEntityRecord = m_EntityRecords[archetypeRecord.Storage.GetEntityIndices().back()];

		archetypeRecord.Storage.RemoveEntityData(entityBufferIndex);
		lastEntityRecord.BufferIndex = entityBufferIndex;
	}
}