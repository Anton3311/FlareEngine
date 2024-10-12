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

#include "FlareECS/Entities.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	class QueryChunk
	{
	public:
		QueryChunk() = default;
		constexpr QueryChunk(const Entity* idArray, size_t entityCount)
			: m_IdArray(idArray), m_EntityCount(entityCount) {}

		inline Entity GetEntityId(size_t entityIndex) const
		{
			FLARE_CORE_ASSERT(entityIndex < m_EntityCount);
			return m_IdArray[entityIndex];
		}

		constexpr size_t GetEntityCount() const { return m_EntityCount; }
	private:
		size_t m_EntityCount = 0;
		const Entity* m_IdArray;
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

	//
	// QueryIterator
	//

	template<typename T>
	struct QueryIteratorArgument
	{
		static void Create(const QueryChunk& chunk,
			const ArchetypeRecord& archetype,
			size_t chunkIndex,
			EntityStorage& storage,
			T& outArgument) {}
	};

	template<>
	struct QueryIteratorArgument<QueryChunk>
	{
		static void Create(const QueryChunk& chunk,
			const ArchetypeRecord& archetype,
			size_t chunkIndex,
			EntityStorage& storage,
			QueryChunk& outArgument)
		{
			outArgument = chunk;
		}
	};

	template<typename T>
	struct QueryIteratorArgument<ComponentView<T>>
	{
		static void Create(const QueryChunk& chunk,
			const ArchetypeRecord& archetype,
			size_t chunkIndex,
			EntityStorage& storage,
			ComponentView<T>& outArgument)
		{
			ComponentId componentId = COMPONENT_ID(std::remove_reference_t<T>);

			std::optional<size_t> componentIndex = archetype.TryGetComponentIndex(componentId);
			FLARE_CORE_ASSERT(componentIndex.has_value());

			T* componentArray = (T*)storage.GetComponentArray(chunkIndex, *componentIndex);
			outArgument = ComponentView<T>(componentArray);
		}
	};

	template<typename... Args>
	struct QueryIterationHelper
	{
		using TupleT = std::tuple<Args...>;

		static TupleT CreateIteratorArguments(QueryChunk chunk,
			const ArchetypeRecord& archetype,
			size_t chunkIndex,
			EntityStorage& storage)
		{
			return TupleT();
		}
	};

	template<typename... Args>
	struct QueryIterationHelper<ArgumentsList<Args...>>
	{
		using TupleT = std::tuple<Args...>;

		static TupleT CreateIteratorArguments(QueryChunk chunk,
			const ArchetypeRecord& archetype,
			size_t chunkIndex,
			EntityStorage& storage)
		{
			FLARE_PROFILE_FUNCTION();

			TupleT tuple;

			// NOTE: When generating function arguments for std::make_tuple using a lambda with fold expression,
			//       the arguments are being generated in reverse order, which results in wrong component offsets for ComponentViews.

			([&]()
				{
					QueryIteratorArgument<Args>::Create(chunk, archetype, chunkIndex, storage, std::get<Args>(tuple));
				} (), ...);

			return tuple;
		}
	};

	//
	// Query
	//

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
			using IteratorArguments = typename IteratorTraits::Arguments;
			using IterationHelper = QueryIterationHelper<IteratorArguments>;
			using FirstArg = typename FirstArgument<IteratorArguments>::Type;

			using FirstArgType = std::remove_const_t<std::remove_reference_t<FirstArg>>;

			const QueryData& queryData = m_Queries->GetQueryData(m_Id);

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
				for (size_t chunkIndex = 0; chunkIndex < storage->GetChunkCount(); chunkIndex++)
				{
					auto arguments = IterationHelper::CreateIteratorArguments(
						QueryChunk(storage->GetReadonlyEntityIdsArray(chunkIndex), storage->GetEntitiesCountInChunk(chunkIndex)),
						archetype,
						chunkIndex,
						*storage);

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