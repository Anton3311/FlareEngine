#pragma once

#include "FlareCore/FunctionTraits.h"
#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Archetype.h"

#include "FlareECS/EntityStorage/EntityStorage.h"

#include "FlareECS/Query/ComponentView.h"
#include "FlareECS/Query/QueryCache.h"
#include "FlareECS/Query/QueryData.h"
#include "FlareECS/Query/QueryChunkEntity.h"

#include "FlareECS/Entities.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	class QueryChunkIterator
	{
	public:
		constexpr QueryChunkIterator(EntityDataGetter& dataGetter, size_t entityIndex)
			: m_DataGetter(dataGetter), m_EntityIndex(entityIndex) {}

		constexpr QueryChunkEntity operator*() { return QueryChunkEntity(m_DataGetter.GetData(m_EntityIndex)); }

		constexpr QueryChunkIterator& operator++()
		{
			m_EntityIndex++;
			return *this;
		}

		constexpr bool operator==(const QueryChunkIterator& other)
		{
			return &m_DataGetter == &other.m_DataGetter && m_EntityIndex == other.m_EntityIndex;
		}

		constexpr bool operator!=(const QueryChunkIterator& other)
		{
			return &m_DataGetter != &other.m_DataGetter || m_EntityIndex != other.m_EntityIndex;
		}
	private:
		EntityDataGetter& m_DataGetter;
		size_t m_EntityIndex = 0;
	};

	class QueryChunk
	{
	public:
		QueryChunk() = default;
		QueryChunk(EntityDataGetter&& dataGetter, EntityIdGetter&& idGetter, size_t entityCount)
			: m_DataGetter(dataGetter), m_IdGetter(idGetter), m_EntityCount(entityCount) {}

		inline Entity GetEntityId(size_t entityIndex) const
		{
			FLARE_CORE_ASSERT(entityIndex < m_EntityCount);
			return m_IdGetter.GetEntityId(entityIndex);
		}

		constexpr size_t GetEntityCount() const { return m_EntityCount; }
		constexpr QueryChunkIterator begin() { return QueryChunkIterator(m_DataGetter, 0); }
		constexpr QueryChunkIterator end() { return QueryChunkIterator(m_DataGetter, m_EntityCount); }
	private:
		size_t m_EntityCount = 0;
		EntityDataGetter m_DataGetter;
		EntityIdGetter m_IdGetter;
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
			FLARE_PROFILE_FUNCTION();
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
			FLARE_PROFILE_FUNCTION();
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
		virtual std::optional<Entity> TryGetFirstEntityId() const override;
		virtual size_t GetEntitiesCount() const override;

		template<typename IteratorFunction>
		inline void ForEachChunk(const IteratorFunction& function)
		{
			FLARE_PROFILE_FUNCTION();
			using IteratorTraits = FunctionTraits<IteratorFunction>;
			static_assert(IteratorTraits::ArgumentsCount >= 2, "A query iterator function must accept a QueryChunk as the first argument and at least 1 component view");

			using IteratorArguments = typename IteratorTraits::Arguments;
			using IterationHelper = QueryIterationHelper<IteratorArguments>;
			using FirstArg = FirstArgument<IteratorArguments>::Type;

			using FirstArgType = std::remove_const_t<std::remove_reference_t<FirstArg>>;

			// QueryChunk + at least 1 component view
			static_assert(std::is_same_v<FirstArgType, QueryChunk>);

			const QueryData& queryData = m_Queries->GetQueryData(m_Id);

			size_t componentOffsets[IteratorTraits::ArgumentsCount];
			const Archetypes& archetypes = m_Entities->GetArchetypes();
			for (ArchetypeId matchedArchetype : GetMatchingArchetypes())
			{
				EntityStorage* storage = nullptr;
				
				switch (queryData.Target)
				{
				case QueryTarget::AllEntities:
					storage = &m_Entities->GetEntityStorage(matchedArchetype);
					break;
				case QueryTarget::DeletedEntities:
					storage = &m_Entities->GetDeletedEntityStorage(matchedArchetype);
					break;
				default:
					FLARE_CORE_ASSERT(false);
				}

				const ArchetypeRecord& archetype = archetypes[matchedArchetype];

				IterationHelper::FillComponentOffsets(componentOffsets, archetype, archetypes);
				for (size_t chunkIndex = 0; chunkIndex < storage->GetChunkCount(); chunkIndex++)
				{
					auto arguments = IterationHelper::Get(
						QueryChunk(storage->CreateEntityDataGetter(chunkIndex),
							storage->CreateEntityIdGetter(chunkIndex),
							storage->GetEntitiesCountInChunk(chunkIndex)),
						componentOffsets);

					{
						FLARE_PROFILE_SCOPE("IterateChunk");
						std::apply(function, arguments);
					}
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