#include "QueyrBuilder.h"

namespace Flare
{
	QueryBuilder::QueryBuilder(QueryCache& queries)
		: m_Queries(queries) {}

	Query QueryBuilder::Create()
	{
		return m_Queries.AddQuery(ComponentSet(m_Ids));
	}
}
