#include "Query.h"

namespace Flare
{
	QueryId Query::GetId() const
	{
		return m_Id;
	}

	QueryIterator Query::begin() const
	{
		FLARE_CORE_ASSERT(m_Entities);
		FLARE_CORE_ASSERT(m_QueryCache);

		const auto& data = (*m_QueryCache)[m_Id];
		return QueryIterator(*m_Entities, data.Target, data.MatchedArchetypes.begin());
	}

	QueryIterator Query::end() const
	{
		FLARE_CORE_ASSERT(m_Entities);
		FLARE_CORE_ASSERT(m_QueryCache);
		const auto& data = (*m_QueryCache)[m_Id];
		return QueryIterator(*m_Entities, data.Target, data.MatchedArchetypes.end());
	}

	const std::unordered_set<ArchetypeId>& Query::GetMatchedArchetypes() const
	{
		return (*m_QueryCache)[m_Id].MatchedArchetypes;
	}

	size_t Query::GetEntitiesCount() const
	{
		size_t count = 0;
		for (ArchetypeId archetype : (*m_QueryCache)[m_Id].MatchedArchetypes)
			count += m_Entities->GetEntityStorage(archetype).GetEntitiesCount();
		
		return count;
	}



	size_t CreatedEntitiesQuery::GetEntitiesCount() const
	{
		size_t count = 0;
		const QueryData& queryData = (*m_QueryCache)[m_Id];
		for (ArchetypeId archetype : queryData.MatchedArchetypes)
		{
			Span<Entity> ids = m_Entities->GetCreateEntities(archetype);
			count += ids.GetSize();
		}

		return count;
	}
}