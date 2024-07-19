#pragma once

#include "FlareCore/FunctionTraits.h"

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Archetype.h"

#include "FlareECS/Query/QueryCache.h"
#include "FlareECS/Query/QueryData.h"
#include "FlareECS/Query/EntityView.h"

#include "FlareECS/Entities.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	class QueryIterator
	{
	public:
		QueryIterator(Entities& entities, QueryTarget target, const std::unordered_set<ArchetypeId>::const_iterator& archetype)
			: m_Entities(entities), m_Target(target), m_Archetype(archetype) {}

		inline EntityView operator*()
		{
			return EntityView(m_Entities, m_Target, *m_Archetype);
		}

		inline QueryIterator operator++()
		{
			m_Archetype++;
			return *this;
		}

		inline bool operator==(const QueryIterator& other)
		{
			return &m_Entities == &other.m_Entities && m_Archetype == other.m_Archetype;
		}

		inline bool operator!=(const QueryIterator& other)
		{
			return &m_Entities != &other.m_Entities || m_Archetype != other.m_Archetype;
		}
	private:
		Entities& m_Entities;
		QueryTarget m_Target;
		std::unordered_set<ArchetypeId>::const_iterator m_Archetype;
	};

	class QueryChunkIterator
	{
	public:
		QueryChunkIterator(uint8_t* entityData, size_t entitySize)
			: m_EntityData(entityData), m_EntitySize(entitySize) {}

		inline EntityViewElement operator*() { return EntityViewElement(m_EntityData); }

		inline QueryChunkIterator& operator++()
		{
			m_EntityData += m_EntitySize;
			return *this;
		}

		inline bool operator==(const QueryChunkIterator& other)
		{
			return m_EntityData == other.m_EntityData;
		}

		inline bool operator!=(const QueryChunkIterator& other)
		{
			return m_EntityData != other.m_EntityData;
		}
	private:
		uint8_t* m_EntityData;
		size_t m_EntitySize;
	};

	class QueryChunk
	{
	public:
		QueryChunk() = default;
		QueryChunk(uint8_t* chunkData, size_t entitiesCount, size_t entitySize)
			: m_ChunkData(chunkData), m_EntitiesCount(entitiesCount), m_EntitySize(entitySize) {}

		inline QueryChunkIterator begin() const { return QueryChunkIterator(m_ChunkData, m_EntitySize); }
		inline QueryChunkIterator end() const { return QueryChunkIterator(m_ChunkData + m_EntitySize * m_EntitiesCount, m_EntitySize); }
	private:
		uint8_t* m_ChunkData = nullptr;
		size_t m_EntitiesCount = 0;
		size_t m_EntitySize = 0;
	};

	class FLAREECS_API EntitiesQuery
	{
	public:
		EntitiesQuery() = default;
		constexpr EntitiesQuery(QueryId id, const QueryCache& queries, Entities& entities)
			: m_Id(id), m_Queries(&queries), m_Entities(&entities) {}

		virtual ~EntitiesQuery() {}

		virtual std::optional<Entity> TryGetFirstEntityId() const = 0;
		virtual size_t GetEntitiesCount() const = 0;

		inline QueryId GetId() const { return m_Id; }
		const std::unordered_set<ArchetypeId>& GetMatchingArchetypes() const { return m_Queries->GetQueryData(m_Id).MatchedArchetypes; }
	protected:
		QueryId m_Id = INVALID_QUERY_ID;
		const QueryCache* m_Queries = nullptr;
		Entities* m_Entities = nullptr;
	};

	template<typename T>
	struct QueryIterationHelper
	{
		static std::tuple<QueryChunk> Get(QueryChunk chunk, const size_t* componentOffset)
		{
			return std::make_tuple(chunk);
		}

		static void FillComponentOffsets(size_t* offsets, const ArchetypeRecord& archetype, const Archetypes& archetypes)
		{
		
		}
	};

	template<typename FirstArg, typename... Args>
	struct QueryIterationHelper<ArgumentsList<FirstArg, Args...>>
	{
		static std::tuple<QueryChunk, Args...> Get(QueryChunk chunk, const size_t* componentOffsets)
		{
			size_t componentIndex = 0;

			std::tuple<QueryChunk, Args...> tuple;
			std::get<QueryChunk>(tuple) = chunk;

			// NOTE: When generating function arguments for std::make_tuple using a lambda with fold expression,
			//       the arguments are being generated in reverse order, which results in wrong component offsets for ComponentViews.

			([&]()
				{
					static_assert(IsComponentView<Args>);
					std::get<Args>(tuple) = Args(componentOffsets[componentIndex++]);
				} (), ...);

			return tuple;
		}

		static void FillComponentOffsets(size_t* offsets, const ArchetypeRecord& archetype, const Archetypes& archetypes)
		{
			size_t index = 0;
			([&]()
				{
					static_assert(IsComponentView<Args>);
					ComponentId componentId = COMPONENT_ID(std::remove_reference_t<typename ComponentViewUnderlyingType<Args>::Type>);
					std::optional<size_t> componentIndex = archetype.TryGetComponentIndex(componentId);
					if (componentIndex)
						offsets[index] = archetype.ComponentOffsets[*componentIndex];
					index++;
				} (), ...);
		}
	};

	class FLAREECS_API Query : public EntitiesQuery
	{
	public:
		Query() = default;
		constexpr Query(QueryId id, Entities& entities, const QueryCache& queries)
			: EntitiesQuery(id, queries, entities) {}
	public:
		QueryIterator begin() const;
		QueryIterator end() const;

		virtual std::optional<Entity> TryGetFirstEntityId() const override;
		virtual size_t GetEntitiesCount() const override;

		template<typename IteratorFunction>
		inline void ForEachChunk(const IteratorFunction& function)
		{
			using IteratorTraits = FunctionTraits<IteratorFunction>;
			static_assert(IteratorTraits::ArgumentsCount >= 2, "A query iterator function must accept a QueryChunk as the first agument and at least 1 component view");

			using IteratorArguments = typename IteratorTraits::Arguments;
			using IterationHelper = QueryIterationHelper<IteratorArguments>;
			using FirstArg = FirstArgument<IteratorArguments>::Type;

			using FirstArgType = std::remove_const_t<std::remove_reference_t<FirstArg>>;

			// QueryChynk + at least 1 component view
			static_assert(std::is_same_v<FirstArgType, QueryChunk>);

			size_t componentOffsets[IteratorTraits::ArgumentsCount];
			const Archetypes& archetypes = m_Entities->GetArchetypes();
			for (ArchetypeId matchedArchetype : GetMatchingArchetypes())
			{
				EntityStorage& storage = m_Entities->GetEntityStorage(matchedArchetype);
				const ArchetypeRecord& archetype = archetypes[matchedArchetype];

				IterationHelper::FillComponentOffsets(componentOffsets, archetype, archetypes);
				for (size_t chunkIndex = 0; chunkIndex < storage.GetChunksCount(); chunkIndex++)
				{
					uint8_t* entityData = storage.GetChunkBuffer(chunkIndex);
					auto arguments = IterationHelper::Get(
						QueryChunk(entityData, storage.GetEntitiesCountInChunk(chunkIndex), storage.GetEntitySize()),
						componentOffsets);

					std::apply(function, arguments);
				}
			}
		}
	};

	class FLAREECS_API CreatedEntitiesQuery : public EntitiesQuery
	{
	public:
		CreatedEntitiesQuery() = default;
		constexpr CreatedEntitiesQuery(QueryId id, Entities& entities, const QueryCache& queries)
			: EntitiesQuery(id, queries, entities) {}

		template<typename IteratorFunction>
		void ForEachEntity(const IteratorFunction& iterator) const
		{
			for (ArchetypeId archetype : GetMatchingArchetypes())
			{
				Span<Entity> ids = m_Entities->GetCreatedEntities(archetype);

				for (Entity id : ids)
					iterator(id);
			}
		}

		virtual std::optional<Entity> TryGetFirstEntityId() const override;
		virtual size_t GetEntitiesCount() const override;
	};
}