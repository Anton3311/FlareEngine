#include "Archetypes.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"

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
			ArchetypeId id = Records.size();
			ArchetypeRecord& record = Records.emplace_back();
			record.Id = id;
			record.Components = std::move(idsCopy);
			record.CreatedEntitiesQueryReferences = 0;
			record.DeletionQueryReferences = 0;

			size_t offset = 0;
			for (size_t i = 0; i < record.Components.size(); i++)
			{
				record.ComponentOffsets[i] = offset;
				offset += m_ComponentsRegistry.GetComponentInfo(record.Components[i]).Size;
			}

			return &record;
		}

		return &Records[it->second];
	}

	ArchetypeId Archetypes::CreateArchetype(Span<const ComponentId> sortedComponentIds)
	{
		return CreateArchetype(std::vector<ComponentId>(sortedComponentIds.begin(), sortedComponentIds.end()));
	}

	ArchetypeId Archetypes::CreateArchetype(std::vector<ComponentId>&& sortedComponentIds)
	{
		FLARE_CORE_ASSERT(sortedComponentIds.size() > 0);

		ArchetypeId archetypeId = Records.size();
		ArchetypeRecord& record = Records.emplace_back();
		record.Id = archetypeId;
		record.CreatedEntitiesQueryReferences = 0;
		record.DeletionQueryReferences = 0;
		record.Components = sortedComponentIds;
		record.ComponentOffsets.resize(record.Components.size());

		size_t entitySize = 0;
		size_t offset = 0;
		for (size_t i = 0; i < record.Components.size(); i++)
		{
			size_t componentSize = m_ComponentsRegistry.GetComponentInfo(record.Components[i]).Size;
			record.ComponentOffsets[i] = offset;
			offset += componentSize;
			entitySize += componentSize;
		}

		record.EntitySize = entitySize;

		ComponentSetToArchetype[ComponentSet(record.Components)] = archetypeId;

		for (size_t i = 0; i < record.Components.size(); i++)
		{
			ComponentId component = record.Components[i];
			ComponentToArchetype[component].emplace(archetypeId, i);
		}

		return archetypeId;
	}
}
