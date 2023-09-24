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
		QueryIterator(Entities& entities, const std::unordered_set<ArchetypeId>::const_iterator& archetype)
			: m_Entities(entities), m_Archetype(archetype) {}

		inline EntityView operator*()
		{
			return EntityView(m_Entities, *m_Archetype);
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
		std::unordered_set<ArchetypeId>::const_iterator m_Archetype;
	};

	class FLAREECS_API Query
	{
	public:
		constexpr Query()
			: m_Id(INVALID_QUERY_ID), m_Entities(nullptr), m_QueryCache(nullptr) {}
		constexpr Query(QueryId id, Entities& entities, const QueryCache& queries)
			: m_Id(id), m_Entities(&entities), m_QueryCache(&queries) {}
	public:
		QueryId GetId() const;

		QueryIterator begin() const;
		QueryIterator end() const;
		const std::unordered_set<ArchetypeId>& GetMatchedArchetypes() const;

		size_t GetEntitiesCount() const;
	private:
		QueryId m_Id;
		Entities* m_Entities;
		const QueryCache* m_QueryCache;
	};
}