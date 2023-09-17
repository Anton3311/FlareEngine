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
		return QueryIterator(*m_Entities, (*m_QueryCache)[m_Id].MatchedArchetypes.begin());
	}

	QueryIterator Query::end() const
	{
		FLARE_CORE_ASSERT(m_Entities);
		FLARE_CORE_ASSERT(m_QueryCache);
		return QueryIterator(*m_Entities, (*m_QueryCache)[m_Id].MatchedArchetypes.end());
	}

	const std::unordered_set<ArchetypeId>& Query::GetMatchedArchetypes() const
	{
		return (*m_QueryCache)[m_Id].MatchedArchetypes;
	}
}