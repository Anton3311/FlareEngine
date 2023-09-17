#pragma once

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Archetype.h"

#include "FlareECS/Query/QueryCache.h"
#include "FlareECS/Query/QueryData.h"
#include "FlareECS/Query/EntityView.h"

#include "FlareECS/Registry.h"

#include <vector>
#include <unordered_set>

namespace Flare
{
	class QueryIterator
	{
	public:
		QueryIterator(Registry& registry, const std::unordered_set<ArchetypeId>::const_iterator& archetype)
			: m_Registry(registry), m_Archetype(archetype) {}

		inline EntityView operator*()
		{
			return EntityView(m_Registry, *m_Archetype);
		}

		inline QueryIterator operator++()
		{
			m_Archetype++;
			return *this;
		}

		inline bool operator==(const QueryIterator& other)
		{
			return &m_Registry == &other.m_Registry && m_Archetype == other.m_Archetype;
		}

		inline bool operator!=(const QueryIterator& other)
		{
			return &m_Registry != &other.m_Registry || m_Archetype != other.m_Archetype;
		}
	private:
		Registry& m_Registry;
		std::unordered_set<ArchetypeId>::const_iterator m_Archetype;
	};

	class FLAREECS_API Query
	{
	public:
		constexpr Query()
			: m_Id(INVALID_QUERY_ID), m_Registry(nullptr), m_QueryCache(nullptr) {}
		constexpr Query(QueryId id, Registry& registry, const QueryCache& queries)
			: m_Id(id), m_Registry(&registry), m_QueryCache(&queries) {}
	public:
		QueryId GetId() const;

		QueryIterator begin() const;
		QueryIterator end() const;
		const std::unordered_set<ArchetypeId>& GetMatchedArchetypes() const;
	private:
		QueryId m_Id;
		Registry* m_Registry;
		const QueryCache* m_QueryCache;
	};
}