#include "QueyrBuilder.h"

namespace Flare
{
	template<>
	FLAREECS_API Query QueryBuilder<Query>::Build()
	{
		return m_Queries.CreateQuery(m_Data);
	}

	template<>
	FLAREECS_API CreatedEntitiesQuery QueryBuilder<CreatedEntitiesQuery>::Build()
	{
		Query query = m_Queries.CreateQuery(m_Data);
		return CreatedEntitiesQuery(query.GetId(), &m_Queries.GetEntitites(), &m_Queries);
	}
}
