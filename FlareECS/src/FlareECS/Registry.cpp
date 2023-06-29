#include "Registry.h"

#include "FlareECS/Query/EntityView.h"

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
		}

		ArchetypeRecord& archetypeRecord = m_Archetypes[record.Archetype];
		record.BufferIndex = archetypeRecord.Storage.AddEntity(record.RegistryIndex);

		Entity entity = Entity((uint32_t)registryIndex);
		m_EntityToRecord.emplace(entity, record.RegistryIndex);
		return entity;
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
		if (archetype.Data.FindComponent(componentId).has_value())
			return false;

		size_t oldComponentCount = archetype.Data.Components.size();
		ArchetypeId newArchetypeId = INVALID_ARCHETYPE_ID;

		size_t insertedComponentIndex = SIZE_MAX;

		auto edgeIterator = archetype.Data.Edges.find(componentId);
		if (edgeIterator != archetype.Data.Edges.end())
		{
			newArchetypeId = edgeIterator->second.Add;
			
			std::optional<size_t> index = m_Archetypes[newArchetypeId].Data.FindComponent(componentId);
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
			for (; insertedComponentIndex > 0; insertedComponentIndex--)
			{
				if (newComponents[insertedComponentIndex - 1] > newComponents[insertedComponentIndex])
				{
					std::swap(newComponents[insertedComponentIndex - 1], newComponents[insertedComponentIndex]);
					insertedComponentIndex--;
				}
				else
				{
					break;
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

			m_Archetypes[entityRecord.Archetype].Data.Edges.emplace(componentId, ArchetypeEdge{newArchetypeId, INVALID_ARCHETYPE_ID});
			m_Archetypes[newArchetypeId].Data.Edges.emplace(componentId, ArchetypeEdge{INVALID_ARCHETYPE_ID, entityRecord.Archetype});
		}

		FLARE_CORE_ASSERT(insertedComponentIndex != SIZE_MAX);

		ArchetypeRecord& oldArchetype = m_Archetypes[entityRecord.Archetype];
		ArchetypeRecord& newArchetype = m_Archetypes[newArchetypeId];

		size_t newEntityIndex = newArchetype.Storage.AddEntity(entityRecord.RegistryIndex);
		uint8_t* newEntityData = newArchetype.Storage.GetEntityData(newEntityIndex);
		const uint8_t* oldEntityData = oldArchetype.Storage.GetEntityData(entityRecord.BufferIndex);

		size_t componentSize = m_RegisteredComponents[componentId].Size;
		size_t sizeBefore = newArchetype.Data.ComponentOffsets[insertedComponentIndex];
		size_t sizeAfter = newArchetype.Storage.GetEntitySize() - (sizeBefore + componentSize);

		std::memcpy(newEntityData, oldEntityData, sizeBefore);
		std::memcpy(newEntityData + sizeBefore, componentData, componentSize);
		std::memcpy(newEntityData + sizeBefore + componentSize, oldEntityData + sizeBefore, sizeAfter);

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

		std::optional<size_t> componentIndex = archetype.Data.FindComponent(component);
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

	EntityView Registry::View(ComponentSet components)
	{
		std::sort(components.GetIds(), components.GetIds() + components.GetCount());
		auto it = m_ComponentSetToArchetype.find(components);

		FLARE_CORE_ASSERT(it != m_ComponentSetToArchetype.end());

		size_t archetypeId = it->second;
		return EntityView(*this, archetypeId);
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

	void Registry::RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex)
	{
		ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];
		EntityRecord& lastEntityRecord = m_EntityRecords[archetypeRecord.Storage.GetEntityIndices().back()];

		archetypeRecord.Storage.RemoveEntityData(entityBufferIndex);
		lastEntityRecord.BufferIndex = entityBufferIndex;
	}
}