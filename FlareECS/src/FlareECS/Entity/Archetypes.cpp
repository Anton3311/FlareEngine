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
		FLARE_PROFILE_FUNCTION();
		if (componentCount <= INLINE_BUFFER_CAPACITY)
		{
			ComponentIds = m_InlineComponentIds;
			ComponentArrayOffsets = m_InlineComponentArrayOffset;
		}
		else
		{
			ComponentIds = new ComponentId[componentCount];
			ComponentArrayOffsets = new EntitySizeT[componentCount];
		}
	}

	ArchetypeComponents::~ArchetypeComponents()
	{
		if (!IsUsingInlineBuffers())
		{
			delete[] ComponentIds;
			delete[] ComponentArrayOffsets;
		}
	}

	ArchetypeComponents::ArchetypeComponents(ArchetypeComponents&& other) noexcept
		: ComponentIds(m_InlineComponentIds),
		ComponentArrayOffsets(m_InlineComponentArrayOffset),
		IdsBufferOffset(other.IdsBufferOffset),
		ComponentCount(other.ComponentCount)
	{
		if (other.IsUsingInlineBuffers())
		{
			std::memcpy(m_InlineComponentIds, other.m_InlineComponentIds, sizeof(ComponentId) * INLINE_BUFFER_CAPACITY);
			std::memcpy(m_InlineComponentArrayOffset, other.m_InlineComponentArrayOffset, sizeof(EntitySizeT) * INLINE_BUFFER_CAPACITY);
		}
		else
		{
			ComponentIds = other.ComponentIds;
			ComponentArrayOffsets = other.ComponentArrayOffsets;

			other.ComponentIds = other.m_InlineComponentIds;
			other.ComponentArrayOffsets = other.m_InlineComponentArrayOffset;
		}

		other.ComponentCount = 0;
		other.IdsBufferOffset = 0;
	}

	ArchetypeComponents& ArchetypeComponents::operator=(ArchetypeComponents&& other) noexcept
	{
		ComponentIds = m_InlineComponentIds;
		ComponentArrayOffsets = m_InlineComponentArrayOffset;
		IdsBufferOffset = other.IdsBufferOffset;
		ComponentCount = other.ComponentCount;

		if (other.IsUsingInlineBuffers())
		{
			std::memcpy(m_InlineComponentIds, other.m_InlineComponentIds, sizeof(ComponentId) * INLINE_BUFFER_CAPACITY);
			std::memcpy(m_InlineComponentArrayOffset, other.m_InlineComponentArrayOffset, sizeof(EntitySizeT) * INLINE_BUFFER_CAPACITY);
		}
		else
		{
			delete[] ComponentIds;
			delete[] ComponentArrayOffsets;

			ComponentIds = other.ComponentIds;
			ComponentArrayOffsets = other.ComponentArrayOffsets;

			other.ComponentIds = other.m_InlineComponentIds;
			other.ComponentArrayOffsets = other.m_InlineComponentArrayOffset;
		}

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

	EntitySizeT ArchetypeComponents::FindComponentIndex(ComponentId component) const
	{
		FLARE_PROFILE_FUNCTION();

		EntitySizeT left = INLINE_BUFFER_CAPACITY;
		EntitySizeT right = ComponentCount;

		while (right - left > 1)
		{
			EntitySizeT mid = (left + right) / 2;
			if (ComponentIds[mid] == component)
				return mid;
			else if (ComponentIds[mid] > component)
				right = mid;
			else
				left = mid;
		}

		if (ComponentIds[left] == component)
			return left;
		
		return INVALID_COMPONENT_INDEX;
	}

	//
	// Archetypes
	//

	Archetypes::Archetypes(Components& componentsRegistry)
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

	void Archetypes::Initialize()
	{
		m_ComponentsRegistry.AddUpdateHandler(*this);
	}

	void Archetypes::Uninitialize()
	{
		m_ComponentsRegistry.RemoveUpdateHandler(*this);
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

		auto it = m_ComponentSetToArchetype.find(ComponentSet::FromVector(idsCopy));
		if (it == m_ComponentSetToArchetype.end())
		{
			return nullptr;
		}

		return &m_Records[it->second.GetValue()];
	}

	const ArchetypeRecord* Archetypes::FindOrCreateArchetype(Span<const ComponentId> components)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<ComponentId> idsCopy(components.begin(), components.end());
		std::sort(idsCopy.begin(), idsCopy.end());

		auto it = m_ComponentSetToArchetype.find(ComponentSet::FromVector(idsCopy));
		if (it == m_ComponentSetToArchetype.end())
		{
			ArchetypeId archetypeId = CreateArchetype(std::move(idsCopy));
			return &m_Records[archetypeId.GetValue()];
		}

		return &m_Records[it->second.GetValue()];
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

		ArchetypeId archetypeId = GetNextArchetypeId();
		size_t archetypeRegistryIndex = m_Records.size();

		ArchetypeRecord& record = m_Records.emplace_back();
		record.Id = archetypeId;
		record.CreatedEntitiesQueryReferences = 0;
		record.DeletionQueryReferences = 0;

		m_ArchetypeIdToIndex[archetypeId] = archetypeRegistryIndex;

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

		auto& archetypeEdges = m_Records[archetype.GetValue()].Edges;

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

	void Archetypes::DeleteArchetype(ArchetypeId archetype)
	{
		FLARE_PROFILE_FUNCTION();

		auto it = m_ArchetypeIdToIndex.find(archetype);
		FLARE_CORE_ASSERT(it != m_ArchetypeIdToIndex.end());

		size_t registryIndex = it->second;

		if (registryIndex != m_Records.size() - 1)
		{
			ArchetypeId lastArchetypeId = m_Records.back().Id;

			m_Records[registryIndex] = std::move(m_Records.back());
			m_ArchetypeComponents[registryIndex] = std::move(m_ArchetypeComponents.back());

			m_ArchetypeIdToIndex[lastArchetypeId] = registryIndex;
		}

		// Remove from the registry and maps

	 	const ArchetypeComponents& components = m_ArchetypeComponents[registryIndex];

		m_ComponentSetToArchetype.erase(components.GetComponentsAsSpan());

		for (auto& [key, mapping] : m_ComponentToArchetype)
		{
			auto mappingIterator = mapping.find(archetype);
			if (mappingIterator != mapping.end())
			{
				mapping.erase(mappingIterator);
			}
		}

		m_Records.pop_back();
		m_ArchetypeComponents.pop_back();

		m_ArchetypeIdToIndex.erase(archetype);
	}

	void Archetypes::OnComponentUnregister(ComponentId component)
	{
		FLARE_PROFILE_FUNCTION();

		for (size_t archetypeIndex = 0; archetypeIndex < m_ArchetypeComponents.size(); archetypeIndex++)
		{
			if (m_ArchetypeComponents[archetypeIndex].TryGetComponentIndex(component) != ArchetypeComponents::INVALID_COMPONENT_INDEX)
			{
				DeleteArchetype(m_Records[archetypeIndex].Id);
			}
		}
	}
}
