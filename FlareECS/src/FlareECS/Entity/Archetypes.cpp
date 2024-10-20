#include "Archetypes.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <algorithm>

namespace Flare
{
	//
	// ArchetypeComponents
	//

	ArchetypeComponents::ArchetypeComponents(EntitySizeT componentCount)
		: ComponentCount(componentCount)
	{
		FLARE_CORE_ASSERT(componentCount <= INLINE_BUFFER_CAPACITY);
		ComponentIds = m_InlineComponentIds;
		ComponentArrayOffsets = m_InlineComponentArrayOffset;
	}

	ArchetypeComponents::ArchetypeComponents(ArchetypeComponents&& other) noexcept
		: ComponentIds(m_InlineComponentIds),
		ComponentArrayOffsets(m_InlineComponentArrayOffset),
		IdsBufferOffset(other.IdsBufferOffset),
		ComponentCount(other.ComponentCount)
	{
		FLARE_CORE_ASSERT(other.IsUsingInlineBuffers());
		std::memcpy(m_InlineComponentIds, other.m_InlineComponentIds, sizeof(ComponentId) * INLINE_BUFFER_CAPACITY);
		std::memcpy(m_InlineComponentArrayOffset, other.m_InlineComponentArrayOffset, sizeof(EntitySizeT) * INLINE_BUFFER_CAPACITY);

		other.ComponentCount = 0;
		other.IdsBufferOffset = 0;
	}

	ArchetypeComponents& ArchetypeComponents::operator=(ArchetypeComponents&& other) noexcept
	{
		FLARE_CORE_ASSERT(other.IsUsingInlineBuffers());
		ComponentIds = m_InlineComponentIds;
		ComponentArrayOffsets = m_InlineComponentArrayOffset;
		IdsBufferOffset = other.IdsBufferOffset;
		ComponentCount = other.ComponentCount;

		std::memcpy(m_InlineComponentIds, other.m_InlineComponentIds, sizeof(ComponentId) * INLINE_BUFFER_CAPACITY);
		std::memcpy(m_InlineComponentArrayOffset, other.m_InlineComponentArrayOffset, sizeof(EntitySizeT) * INLINE_BUFFER_CAPACITY);

		other.ComponentCount = 0;
		other.IdsBufferOffset = 0;

		return *this;
	}

	void ArchetypeComponents::FillComponentIds(Span<const ComponentId> ids)
	{
		std::memcpy(m_InlineComponentIds, ids.GetData(), std::min(ids.GetSize(), INLINE_BUFFER_CAPACITY) * sizeof(ComponentId));

		if (!IsUsingInlineBuffers())
		{
			std::memcpy(ComponentIds, ids.GetData(), ids.GetSize() * sizeof(ComponentId));
		}
	}

	void ArchetypeComponents::FillComponentArrayOffsets(Span<const EntitySizeT> offsets)
	{
		std::memcpy(m_InlineComponentArrayOffset, offsets.GetData(), std::min(offsets.GetSize(), INLINE_BUFFER_CAPACITY) * sizeof(EntitySizeT));

		if (!IsUsingInlineBuffers())
		{
			std::memcpy(ComponentArrayOffsets, offsets.GetData(), offsets.GetSize() * sizeof(EntitySizeT));
		}
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
			ArchetypeId archetypeId = CreateArchetype(std::move(idsCopy));
			return &m_Records[archetypeId];
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

		ArchetypeComponents& archetypeComponents = m_ArchetypeComponents.emplace_back((EntitySizeT)sortedComponentIds.size());
		archetypeComponents.FillComponentIds(Span(sortedComponentIds.data(), sortedComponentIds.size()));

		InitializeRecord(record, archetypeComponents);

		m_ComponentSetToArchetype[ComponentSet(archetypeComponents.ComponentIds, archetypeComponents.ComponentCount)] = archetypeId;

		for (size_t i = 0; i < archetypeComponents.ComponentCount; i++)
		{
			ComponentId component = archetypeComponents.ComponentIds[i];
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

	void Archetypes::InitializeRecord(ArchetypeRecord& archetype, ArchetypeComponents& archetypeComponents)
	{
		FLARE_PROFILE_FUNCTION();

		archetype.ComponentOffsets.resize(archetypeComponents.ComponentCount, 0);
		archetype.CombinedComponentTypeFlags = m_ComponentsRegistry.GetComponentInfo(archetypeComponents.ComponentIds[0]).Initializer->Type.Flags;

		size_t offset = 0;
		for (EntitySizeT i = 0; i < archetypeComponents.ComponentCount; i++)
		{
			const ComponentInfo& info = m_ComponentsRegistry.GetComponentInfo(archetypeComponents.ComponentIds[i]);
			size_t componentSize = info.Size;

			offset = Align(offset, info.Initializer->Type.Alignment);

			archetype.CombinedComponentTypeFlags &= info.Initializer->Type.Flags;
			archetype.ComponentOffsets[i] = (EntitySizeT)offset;

			offset += componentSize;
			archetype.EntitySize += (EntitySizeT)componentSize;
			archetype.EntityAlignment = std::max(archetype.EntityAlignment, (EntitySizeT)info.Initializer->Type.Alignment);
		}
		
		{
			std::vector<EntitySizeT> arrayOffsets(archetypeComponents.ComponentCount, 0);
			size_t offset = 0;

			// There is also an array of Entity ids at the of the chunk, so to this into account
			archetype.EntityCountPerChunk = EntityStorageChunk::CHUNK_SIZE / (archetype.EntitySize + sizeof(Entity));

			for (EntitySizeT componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
			{
				arrayOffsets[componentIndex] = (EntitySizeT)offset;

				const ComponentInfo& info = m_ComponentsRegistry.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
				offset += info.Size * archetype.EntityCountPerChunk;
			}

			archetypeComponents.IdsBufferOffset = (EntitySizeT)offset;

			archetypeComponents.FillComponentArrayOffsets(Span(arrayOffsets.data(), arrayOffsets.size()));
		}

		archetype.EntitySize = (EntitySizeT)Align((size_t)archetype.EntitySize,
			m_ComponentsRegistry.GetComponentInfo(archetypeComponents.ComponentIds[0]).Initializer->Type.Alignment);
	}
}
