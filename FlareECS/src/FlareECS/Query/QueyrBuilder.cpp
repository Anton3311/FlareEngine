#include "QueyrBuilder.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	template<>
	FLAREECS_API Query QueryBuilder<Query>::Build()
	{
		FLARE_PROFILE_FUNCTION();
		return Query(m_Queries.CreateQuery(m_Data), m_Entities, m_Queries);
	}

	template<>
	FLAREECS_API CreatedEntitiesQuery QueryBuilder<CreatedEntitiesQuery>::Build()
	{
		FLARE_PROFILE_FUNCTION();
		return CreatedEntitiesQuery(m_Queries.CreateQuery(m_Data), m_Entities, m_Queries);
	}
}
