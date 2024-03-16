#pragma once

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

	template<typename T>
	using QueryIteratorParamterType = typename T;

	template<typename T>
	inline T GetQueryIteratorParameterValue(size_t entityIndex, size_t componentOffset, uint8_t* entityData, const EntityStorage& storage)
	{
		return *(std::remove_reference_t<T>*)(entityData + componentOffset);
	}

	template<>
	inline Entity GetQueryIteratorParameterValue<Entity>(size_t entityIndex, size_t componentOffset, uint8_t* entityData, const EntityStorage& storage)
	{
		// TODO: implement
		return Entity();
	}

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
		QueryChunk(uint8_t* chunkData, size_t entitiesCount, size_t entitySize)
			: m_ChunkData(chunkData), m_EntitiesCount(entitiesCount), m_EntitySize(entitySize) {}

		inline QueryChunkIterator begin() const { return QueryChunkIterator(m_ChunkData, m_EntitySize); }
		inline QueryChunkIterator end() const { return QueryChunkIterator(m_ChunkData + m_EntitySize * m_EntitiesCount, m_EntitySize); }
	private:
		uint8_t* m_ChunkData;
		size_t m_EntitiesCount;
		size_t m_EntitySize;
	};

	class FLAREECS_API EntitiesQuery
	{
	public:
		constexpr EntitiesQuery() = default;
		constexpr EntitiesQuery(QueryId id, const QueryCache& queries, Entities& entities)
			: m_Id(id), m_Queries(&queries), m_Entities(&entities) {}

		virtual ~EntitiesQuery() {}

		virtual std::optional<Entity> TryGetFirstEntityId() const = 0;
		virtual size_t GetEntitiesCount() const = 0;

		inline QueryId GetId() const { return m_Id; }
		const std::unordered_set<ArchetypeId> GetMatchingArchetypes() const { return m_Queries->GetQueryData(m_Id).MatchedArchetypes; }
	protected:
		QueryId m_Id = INVALID_QUERY_ID;
		const QueryCache* m_Queries = nullptr;
		Entities* m_Entities = nullptr;
	};

	class FLAREECS_API Query : public EntitiesQuery
	{
	public:
		constexpr Query() = default;
		constexpr Query(QueryId id, Entities& entities, const QueryCache& queries)
			: EntitiesQuery(id, queries, entities) {}
	public:
		QueryIterator begin() const;
		QueryIterator end() const;

		virtual std::optional<Entity> TryGetFirstEntityId() const override;
		virtual size_t GetEntitiesCount() const override;

		template<typename... Args, typename IteratorFunction>
		inline void ForEachEntity(const IteratorFunction& function)
		{
			size_t componentOffsets[sizeof...(Args)];
			const Archetypes& archetypes = m_Entities->GetArchetypes();
			for (ArchetypeId matchedArchetype : GetMatchingArchetypes())
			{
				EntityStorage& storage = m_Entities->GetEntityStorage(matchedArchetype);
				const ArchetypeRecord& archetype = archetypes[matchedArchetype];

				size_t index = 0;
				([&]()
				{
					std::optional<size_t> componentIndex = archetypes.GetArchetypeComponentIndex(matchedArchetype, COMPONENT_ID(std::remove_reference_t<Args>));
					if (componentIndex)
						componentOffsets[index] = archetype.ComponentOffsets[*componentIndex];
					index++;
				} (), ...);

				for (size_t chunkIndex = 0; chunkIndex < storage.GetChunksCount(); chunkIndex++)
				{
					uint8_t* entityData = storage.GetChunkBuffer(chunkIndex);
					for (size_t entityIndex = 0; entityIndex < storage.GetEntitiesCountInChunk(chunkIndex); entityIndex++)
					{
						size_t componentIndex = 0;

						std::tuple<QueryIteratorParamterType<Args>...> arguments = std::make_tuple(
							[&]() -> decltype(auto)
							{
								return GetQueryIteratorParameterValue<Args>(entityIndex, componentOffsets[componentIndex++], entityData, storage);
							} () ...
						);

						std::apply(function, arguments);
						entityData += storage.GetEntitySize();
					}
				}
			}
		}

		template<typename... Args, typename IteratorFunction>
		inline void ForEachChunk(const IteratorFunction& function)
		{
			size_t componentOffsets[sizeof...(Args)];
			const Archetypes& archetypes = m_Entities->GetArchetypes();
			for (ArchetypeId matchedArchetype : GetMatchingArchetypes())
			{
				EntityStorage& storage = m_Entities->GetEntityStorage(matchedArchetype);
				const ArchetypeRecord& archetype = archetypes[matchedArchetype];

				size_t index = 0;
				([&]()
				{
					std::optional<size_t> componentIndex = archetypes.GetArchetypeComponentIndex(matchedArchetype, COMPONENT_ID(std::remove_reference_t<Args>));
					if (componentIndex)
						componentOffsets[index] = archetype.ComponentOffsets[*componentIndex];
					index++;
				} (), ...);

				for (size_t chunkIndex = 0; chunkIndex < storage.GetChunksCount(); chunkIndex++)
				{
					uint8_t* entityData = storage.GetChunkBuffer(chunkIndex);
					size_t componentIndex = 0;

					std::tuple<QueryChunk, ComponentView<Args>...> arguments = std::make_tuple(
						QueryChunk(entityData, storage.GetEntitiesCountInChunk(chunkIndex), storage.GetEntitySize()),
						[&]() -> decltype(auto)
						{
							return ComponentView<Args>(componentOffsets[componentIndex++]);
						} () ...
					);

					std::apply(function, arguments);
				}
			}
		}
	};

	class FLAREECS_API CreatedEntitiesQuery : public EntitiesQuery
	{
	public:
		constexpr CreatedEntitiesQuery() = default;
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