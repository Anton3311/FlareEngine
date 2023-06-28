#include "Registry.h"

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
			record.ArchetypeId = it->second;
		else
		{
			ArchetypeRecord& newArchetype = m_Archetypes.emplace_back();
			record.ArchetypeId = newArchetype.Data.Id;

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

		ArchetypeRecord& archetypeRecord = m_Archetypes[record.ArchetypeId];
		record.BufferIndex = archetypeRecord.Storage.AddEntity();

		Entity entity = Entity((uint32_t)registryIndex);
		m_EntityToRecord.emplace(entity, record.RegistryIndex);
		return entity;
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
}