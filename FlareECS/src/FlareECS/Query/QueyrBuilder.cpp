#include "QueyrBuilder.h"

namespace Flare
{
	template<>
	FLAREECS_API Query QueryBuilder<Query>::Build()
	{
		return Query(m_Queries.CreateQuery(m_Data), m_Entities, m_Queries);
	}

	template<>
	FLAREECS_API CreatedEntitiesQuery QueryBuilder<CreatedEntitiesQuery>::Build()
	{
		return CreatedEntitiesQuery(m_Queries.CreateQuery(m_Data), m_Entities, m_Queries);
	}
}
