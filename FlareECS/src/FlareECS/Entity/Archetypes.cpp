#include "Archetypes.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <algorithm>

namespace Flare
{
	//
	// ArchetypeRecord
	//

	std::optional<size_t> ArchetypeRecord::TryGetComponentIndex(ComponentId component) const
	{
		size_t left = 0;
		size_t right = Components.size();

		while (right - left > 1)
		{
			size_t mid = (left + right) / 2;
			if (Components[mid] == component)
				return mid;
			else if (Components[mid] < component)
				left = mid;
			else
				right = mid;
		}

		if (Components[left] == component)
			return left;
		return {};
	}

	//
	// Archetypes
	//

	Archetypes::Archetypes(const Components& componentsRegistry)
		: m_ComponentsRegistry(componentsRegistry)
	{
	}

	Archetypes::~Archetypes()
	{
		FLARE_PROFILE_FUNCTION();
		for (const auto& archetype : m_Records)
		{
			FLARE_CORE_ASSERT(archetype.DeletionQueryReferences == 0 && archetype.CreatedEntitiesQueryReferences == 0);
		}
	}

	void Archetypes::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		m_ComponentSetToArchetype.clear();
		m_ComponentToArchetype.clear();

		m_Records.clear();
	}

	const ArchetypeRecord* Archetypes::FindArchetype(Span<const ComponentId> components) const
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<ComponentId> idsCopy(components.begin(), components.end());
		std::sort(idsCopy.begin(), idsCopy.end());

		auto it = m_ComponentSetToArchetype.find(ComponentSet(idsCopy));
		if (it == m_ComponentSetToArchetype.end())
		{
			return nullptr;
		}

		return &m_Records[it->second];
	}

	const ArchetypeRecord* Archetypes::FindOrCreateArchetype(Span<const ComponentId> components)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<ComponentId> idsCopy(components.begin(), components.end());
		std::sort(idsCopy.begin(), idsCopy.end());

		auto it = m_ComponentSetToArchetype.find(ComponentSet(idsCopy));
		if (it == m_ComponentSetToArchetype.end())
		{
			ArchetypeId archetypeId = (ArchetypeId)m_Records.size();
			ArchetypeRecord& record = m_Records.emplace_back();
			record.Id = archetypeId;
			record.Components = std::move(idsCopy);
			record.CreatedEntitiesQueryReferences = 0;
			record.DeletionQueryReferences = 0;

			m_ComponentSetToArchetype.emplace(ComponentSet(record.Components), archetypeId);

			for (size_t i = 0; i < record.Components.size(); i++)
			{
				ComponentId component = record.Components[i];
				m_ComponentToArchetype[component].emplace(archetypeId, i);
			}

			InitializeRecord(record);

			{
				FLARE_PROFILE_SCOPE("NotifyHandlers");

				for (ArchetypeUpdateHandler* handler : m_UpdateHandlers)
					handler->OnArchetypeCreated(archetypeId);
			}

			return &record;
		}

		return &m_Records[it->second];
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

		ArchetypeId archetypeId = (ArchetypeId)m_Records.size();
		ArchetypeRecord& record = m_Records.emplace_back();
		record.Id = archetypeId;
		record.CreatedEntitiesQueryReferences = 0;
		record.DeletionQueryReferences = 0;
		record.Components = sortedComponentIds;

		InitializeRecord(record);

		m_ComponentSetToArchetype[ComponentSet(record.Components)] = archetypeId;

		for (size_t i = 0; i < record.Components.size(); i++)
		{
			ComponentId component = record.Components[i];
			m_ComponentToArchetype[component].emplace(archetypeId, i);
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

	void Archetypes::AddArchetypeEdge(ArchetypeId archetype, ComponentId component, ArchetypeEdge edge)
	{
		FLARE_PROFILE_FUNCTION();

		auto& archetypeEdges = m_Records[archetype].Edges;

		auto iterator = archetypeEdges.find(component);
		if (iterator == archetypeEdges.end())
		{
			archetypeEdges.emplace(component, edge);
		}
		else
		{
			if (edge.Add != INVALID_ARCHETYPE_ID)
				iterator->second.Add = edge.Add;
			if (edge.Remove != INVALID_ARCHETYPE_ID)
				iterator->second.Remove = edge.Remove;
		}
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
			archetype.ComponentOffsets[i] = (EntitySizeT)offset;

			offset += componentSize;
			archetype.EntitySize += (EntitySizeT)componentSize;
			archetype.EntityAlignment = std::max(archetype.EntityAlignment, (EntitySizeT)info.Initializer->Type.Alignment);
		}
		
		{
			archetype.ComponentArrayOffsets.resize(archetype.Components.size(), 0);

			size_t offset = 0;

			// There is also an array of Entity ids at the of the chunk, so to this into account
			archetype.EntityCountPerChunk = EntityStorageChunk::CHUNK_SIZE / (archetype.EntitySize + sizeof(Entity));

			for (size_t componentIndex = 0; componentIndex < archetype.Components.size(); componentIndex++)
			{
				archetype.ComponentArrayOffsets[componentIndex] = (EntitySizeT)offset;

				const ComponentInfo& info = m_ComponentsRegistry.GetComponentInfo(archetype.Components[componentIndex]);
				offset += info.Size * archetype.EntityCountPerChunk;
			}

			archetype.IdsBufferOffset = (EntitySizeT)offset;
		}

		archetype.EntitySize = (EntitySizeT)Align((size_t)archetype.EntitySize,
			m_ComponentsRegistry.GetComponentInfo(archetype.Components[0]).Initializer->Type.Alignment);
	}
}
