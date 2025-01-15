#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Collections/Span.h"
#include "FlareCore/Log.h"

#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Types.h"
#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Components.h"

#include <bit>
#include <vector>
#include <optional>
#include <unordered_map>

namespace Flare
{
	struct ArchetypeId
	{
	public:
		using UnderlyingType = uint32_t;

		constexpr ArchetypeId()
			: m_Value(std::numeric_limits<UnderlyingType>::max()) { }

		explicit constexpr ArchetypeId(UnderlyingType value)
			: m_Value(value) {}

		constexpr UnderlyingType GetValue() const { return m_Value; }

		constexpr bool operator==(ArchetypeId other) const { return m_Value == other.m_Value; }
		constexpr bool operator!=(ArchetypeId other) const { return m_Value != other.m_Value; }
	private:
		UnderlyingType m_Value;
	};
}

template<>
struct std::hash<Flare::ArchetypeId>
{
	inline size_t operator()(Flare::ArchetypeId id) const
	{
		return std::hash<Flare::ArchetypeId::UnderlyingType>()(id.GetValue());
	}
};

namespace Flare
{

	constexpr ArchetypeId INVALID_ARCHETYPE_ID = ArchetypeId();

	struct ArchetypeEdge
	{
		ArchetypeId Add = INVALID_ARCHETYPE_ID;
		ArchetypeId Remove = INVALID_ARCHETYPE_ID;
	};

	//
	// ArchetypeComponents
	//

	class FLAREECS_API ArchetypeComponents
	{
	public:
		FLARE_NONCOPYABLE(ArchetypeComponents);

		static_assert(std::is_same_v<ComponentId::UnderlyingType, uint16_t>);

		static constexpr size_t INLINE_BUFFER_CAPACITY = 8;
		static constexpr EntitySizeT INVALID_COMPONENT_INDEX = std::numeric_limits<EntitySizeT>::max();

		ArchetypeComponents(EntitySizeT componentCount);
		~ArchetypeComponents();

		ArchetypeComponents(ArchetypeComponents&& other) noexcept;

		ArchetypeComponents& operator=(ArchetypeComponents&& other) noexcept;

		inline EntitySizeT TryGetComponentIndex(ComponentId component) const
		{
			__m128i searchVector = _mm_set1_epi16(component.GetValue());

			const __m128i* idVectors = reinterpret_cast<const __m128i*>(&m_InlineComponentIds);

			__m128i result = _mm_cmpeq_epi16(idVectors[0], searchVector);

			int32_t mask = _mm_movemask_epi8(result);
			EntitySizeT index = (EntitySizeT)(std::countr_zero((uint32_t)mask)) / 2;

			if (index < ComponentCount)
				return index;
			else if (ComponentCount > INLINE_BUFFER_CAPACITY)
				return FindComponentIndex(component);

			return INVALID_COMPONENT_INDEX;
		}

		constexpr bool IsUsingInlineBuffers() const { return ComponentCount <= INLINE_BUFFER_CAPACITY; }

		inline Span<const ComponentId> GetComponentsAsSpan() const { return Span<const ComponentId>(ComponentIds, ComponentCount); }

		void FillComponentIds(Span<const ComponentId> ids);
		void FillComponentArrayOffsets(Span<const EntitySizeT> offsets);
	private:
		EntitySizeT FindComponentIndex(ComponentId component) const;
	public:
		ComponentId* ComponentIds = nullptr;
		EntitySizeT* ComponentArrayOffsets = nullptr;

		EntitySizeT ComponentCount = 0;
		EntitySizeT IdsBufferOffset = 0;
	private:
		alignas(16) ComponentId m_InlineComponentIds[INLINE_BUFFER_CAPACITY];
		alignas(16) EntitySizeT m_InlineComponentArrayOffset[INLINE_BUFFER_CAPACITY] = { 0 };
	};

	//
	// ArchetypeRecord
	//

	class FLAREECS_API ArchetypeRecord
	{
	public:
		FLARE_NONCOPYABLE(ArchetypeRecord);

		ArchetypeRecord() = default;

		ArchetypeRecord(ArchetypeRecord&&) = default;
		ArchetypeRecord& operator=(ArchetypeRecord&&) = default;

		constexpr bool IsUsedInDeletionQuery() const { return DeletionQueryReferences > 0; }
		constexpr bool IsUsedInCreatedEntitiesQuery() const { return CreatedEntitiesQueryReferences > 0; }
	public:
		ArchetypeId Id = INVALID_ARCHETYPE_ID;
		EntitySizeT EntitySize = 0;
		EntitySizeT EntityAlignment = 0;
		EntitySizeT EntityCountPerChunk = 0;

		int32_t DeletionQueryReferences = 0;
		int32_t CreatedEntitiesQueryReferences = 0;

		TypeFlags CombinedComponentTypeFlags = TypeFlags::None;

		std::unordered_map<ComponentId, ArchetypeEdge> Edges;
	};

	//
	// ArchetypeUpdateHandler
	//

	class ArchetypeUpdateHandler
	{
	public:
		virtual void OnArchetypeCreated(ArchetypeId id) = 0;
		virtual void OnArchetypeRemoved(ArchetypeId id) = 0;
	};

	//
	// Archetypes
	//

	struct Components;
	class FLAREECS_API Archetypes : public ComponentsRegistryUpdateHandler
	{
	public:
		FLARE_NONCOPYABLE(Archetypes);

		Archetypes(Components& componentsRegistry);
		~Archetypes();

		void Initialize();
		void Uninitialize();
		void Clear();

		constexpr size_t GetArchetypeCount() const { return m_Records.size(); }

		inline bool IsIdValid(ArchetypeId id) const
		{
			auto it = m_ArchetypeIdToIndex.find(id);
			if (it == m_ArchetypeIdToIndex.end())
				return false;
			return true;
		}

		inline const ArchetypeRecord& operator[](ArchetypeId id) const
		{
			auto it = m_ArchetypeIdToIndex.find(id);
			FLARE_CORE_ASSERT(it != m_ArchetypeIdToIndex.end());
			return m_Records[it->second];
		}

		ArchetypeRecord& GetMutableRecord(ArchetypeId id)
		{
			auto it = m_ArchetypeIdToIndex.find(id);
			FLARE_CORE_ASSERT(it != m_ArchetypeIdToIndex.end());
			return m_Records[it->second];
		}

		Span<const ArchetypeRecord> GetRecords() const { return Span(m_Records.data(), m_Records.size()); }

		const ArchetypeComponents& GetArchetypeComponents(ArchetypeId archetype) const
		{
			auto it = m_ArchetypeIdToIndex.find(archetype);
			FLARE_CORE_ASSERT(it != m_ArchetypeIdToIndex.end());
			return m_ArchetypeComponents[it->second];
		}

		const ArchetypeRecord* FindArchetype(Span<const ComponentId> components) const;
		const ArchetypeRecord* FindOrCreateArchetype(Span<const ComponentId> components);
		
		ArchetypeId CreateArchetype(Span<const ComponentId> sortedComponentIds);
		ArchetypeId CreateArchetype(std::vector<ComponentId>&& sortedComponentIds);

		void AddUpdateHandler(ArchetypeUpdateHandler* handler);
		void RemoveUpdateHandler(ArchetypeUpdateHandler* handler);

		inline const Components& GetCompatibleComponents() const { return m_ComponentsRegistry; }

		void AddArchetypeEdge(ArchetypeId archetype, ComponentId component, ArchetypeEdge edge);

		const std::unordered_map<ArchetypeId, size_t>* GetArchetypesWithComponent(ComponentId component) const
		{
			auto it = m_ComponentToArchetype.find(component);
			if (it == m_ComponentToArchetype.end())
				return nullptr;

			return &it->second;
		}

		void OnComponentUnregister(ComponentId component) override;
	private:
		void InitializeRecord(ArchetypeRecord& archetype, ArchetypeComponents& archetypeComponents);
		void DeleteArchetype(ArchetypeId archetype);

		inline ArchetypeId GetNextArchetypeId()
		{
			ArchetypeId result = m_NextArchetypeId;
			m_NextArchetypeId = ArchetypeId(result.GetValue() + 1);
			return result;
		}
	private:
		Components& m_ComponentsRegistry;

		std::unordered_map<ArchetypeId, size_t> m_ArchetypeIdToIndex;

		std::vector<ArchetypeRecord> m_Records;
		std::vector<ArchetypeComponents> m_ArchetypeComponents;

		std::unordered_map<ComponentSet, ArchetypeId> m_ComponentSetToArchetype;
		std::unordered_map<ComponentId, std::unordered_map<ArchetypeId, size_t>> m_ComponentToArchetype;

		std::vector<ArchetypeUpdateHandler*> m_UpdateHandlers;

		ArchetypeId m_NextArchetypeId = ArchetypeId(0);
	};
}