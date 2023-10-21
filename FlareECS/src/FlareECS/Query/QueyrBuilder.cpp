#include "QueyrBuilder.h"

namespace Flare
{
	QueryBuilder::QueryBuilder(QueryCache& queries)
		: m_Queries(queries) {}

	Query QueryBuilder::Create()
	{
		return m_Queries.CreateQuery(m_Data);
	}
}
