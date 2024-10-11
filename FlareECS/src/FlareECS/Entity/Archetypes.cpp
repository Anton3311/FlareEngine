#include "Archetypes.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <algorithm>

namespace Flare
{
	Archetypes::Archetypes(const Components& componentsRegistry)
		: m_ComponentsRegistry(componentsRegistry)
	{
	}

	Archetypes::~Archetypes()
	{
		FLARE_PROFILE_FUNCTION();
		for (const auto& archetype : Records)
		{
			FLARE_CORE_ASSERT(archetype.DeletionQueryReferences == 0 && archetype.CreatedEntitiesQueryReferences == 0);
		}
	}

	const ArchetypeRecord* Archetypes::FindArchetype(Span<ComponentId> components) const
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<ComponentId> idsCopy(components.begin(), components.end());
		std::sort(idsCopy.begin(), idsCopy.end());

		auto it = ComponentSetToArchetype.find(ComponentSet(idsCopy));
		if (it == ComponentSetToArchetype.end())
		{
			return nullptr;
		}

		return &Records[it->second];
	}

	const ArchetypeRecord* Archetypes::FindOrCreateArchetype(Span<ComponentId> components)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<ComponentId> idsCopy(components.begin(), components.end());
		std::sort(idsCopy.begin(), idsCopy.end());

		auto it = ComponentSetToArchetype.find(ComponentSet(idsCopy));
		if (it == ComponentSetToArchetype.end())
		{
			ArchetypeId archetypeId = Records.size();
			ArchetypeRecord& record = Records.emplace_back();
			record.Id = archetypeId;
			record.Components = std::move(idsCopy);
			record.CreatedEntitiesQueryReferences = 0;
			record.DeletionQueryReferences = 0;

			ComponentSetToArchetype.emplace(ComponentSet(record.Components), archetypeId);

			for (size_t i = 0; i < record.Components.size(); i++)
			{
				ComponentId component = record.Components[i];
				ComponentToArchetype[component].emplace(archetypeId, i);
			}

			InitializeRecord(record);

			{
				FLARE_PROFILE_SCOPE("NotifyHandlers");

				for (ArchetypeUpdateHandler* handler : m_UpdateHandlers)
					handler->OnArchetypeCreated(archetypeId);
			}

			return &record;
		}

		return &Records[it->second];
	}

	ArchetypeId Archetypes::CreateArchetype(Span<const ComponentId> sortedComponentIds)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(sortedComponentIds.GetSize() > 0);
		return CreateArchetype(std::vector<ComponentId>(sortedComponentIds.begin(), sortedComponentIds.end()));
	}
	
	inline static size_t Align(size_t value, size_t alignment)
	{
		return (value + alignment - 1) / alignment * alignment;
	}

	ArchetypeId Archetypes::CreateArchetype(std::vector<ComponentId>&& sortedComponentIds)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(sortedComponentIds.size() > 0);

		ArchetypeId archetypeId = Records.size();
		ArchetypeRecord& record = Records.emplace_back();
		record.Id = archetypeId;
		record.CreatedEntitiesQueryReferences = 0;
		record.DeletionQueryReferences = 0;
		record.Components = sortedComponentIds;

		InitializeRecord(record);

		ComponentSetToArchetype[ComponentSet(record.Components)] = archetypeId;

		for (size_t i = 0; i < record.Components.size(); i++)
		{
			ComponentId component = record.Components[i];
			ComponentToArchetype[component].emplace(archetypeId, i);
		}

		FLARE_CORE_ASSERT(record.EntitySize > 0);
		FLARE_CORE_ASSERT(record.EntityAlignment > 0);

		{
			FLARE_PROFILE_SCOPE("NotifyHandlers");

			for (ArchetypeUpdateHandler* handler : m_UpdateHandlers)
				handler->OnArchetypeCreated(archetypeId);
		}

		return archetypeId;
	}

	void Archetypes::AddUpdateHandler(ArchetypeUpdateHandler* handler)
	{
		m_UpdateHandlers.push_back(handler);
	}

	void Archetypes::RemoveUpdateHandler(ArchetypeUpdateHandler* handler)
	{
		auto it = std::find(m_UpdateHandlers.begin(), m_UpdateHandlers.end(), handler);
		if (it == m_UpdateHandlers.end())
			return;

		m_UpdateHandlers.erase(it);
	}

	void Archetypes::InitializeRecord(ArchetypeRecord& archetype)
	{
		FLARE_PROFILE_FUNCTION();

		archetype.ComponentOffsets.resize(archetype.Components.size(), 0);
		archetype.CombinedComponentTypeFlags = m_ComponentsRegistry.GetComponentInfo(archetype.Components[0]).Initializer->Type.Flags;

		size_t offset = 0;
		for (size_t i = 0; i < archetype.Components.size(); i++)
		{
			const ComponentInfo& info = m_ComponentsRegistry.GetComponentInfo(archetype.Components[i]);
			size_t componentSize = info.Size;

			offset = Align(offset, info.Initializer->Type.Alignment);

			archetype.CombinedComponentTypeFlags &= info.Initializer->Type.Flags;
			archetype.ComponentOffsets[i] = offset;

			offset += componentSize;
			archetype.EntitySize += componentSize;
			archetype.EntityAlignment = std::max(archetype.EntityAlignment, info.Initializer->Type.Alignment);
		}
		
		{
			archetype.ComponentArrayOffsets.resize(archetype.Components.size(), 0);

			size_t offset = 0;
			size_t entityCount = EntityStorageChunk::CHUNK_SIZE / (archetype.EntitySize + 6); // 6 bytes of packed entity id

			for (size_t componentIndex = 0; componentIndex < archetype.Components.size(); componentIndex++)
			{
				archetype.ComponentArrayOffsets[componentIndex] = offset;

				const ComponentInfo& info = m_ComponentsRegistry.GetComponentInfo(archetype.Components[componentIndex]);
				offset += info.Size * entityCount;
			}
		}

		archetype.EntitySize = Align(archetype.EntitySize, m_ComponentsRegistry.GetComponentInfo(archetype.Components[0]).Initializer->Type.Alignment);
	}
}
