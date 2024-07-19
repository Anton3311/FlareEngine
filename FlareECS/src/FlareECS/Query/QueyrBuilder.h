#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Query/QueryCache.h"
#include "FlareECS/Query/QueryFilters.h"
#include "FlareECS/Query/Query.h"

namespace Flare
{
	template<typename T>
	class QueryBuilder
	{
	public:
		QueryBuilder(QueryCache& queries, Entities& entities, QueryTarget target)
			: m_Queries(queries), m_Entities(entities)
		{
			m_Data.Target = target;
		}

		template<typename... T>
		QueryBuilder& With()
		{
			size_t count = sizeof...(T);
			m_Data.Components.reserve(m_Data.Components.size() + count);

			size_t index = 0;

			([&]
			{
				m_Data.Components.push_back(COMPONENT_ID(T));
			} (), ...);

			return *this;
		}

		template<typename... T>
		QueryBuilder& Without()
		{
			size_t count = sizeof...(T);
			m_Data.Components.reserve(m_Data.Components.size() + count);

			size_t index = 0;

			([&]
			{
				ComponentId id = COMPONENT_ID(T);
				m_Data.Components.push_back(ComponentId(
					id.GetIndex() | (uint32_t)QueryFilterType::Without, 
					id.GetGeneration()));
			} (), ...);

			return *this;
		}

		T Build()
		{
			static_assert(false);
		}
	private:
		Entities& m_Entities;
		QueryCache& m_Queries;
		QueryCreationData m_Data;
	};

	template<>
	FLAREECS_API Query QueryBuilder<Query>::Build();
	template<>
	FLAREECS_API CreatedEntitiesQuery QueryBuilder<CreatedEntitiesQuery>::Build();

	class QueryTargetSelector
	{
	public:
		QueryTargetSelector(QueryCache& queries, Entities& entities)
			: m_Queries(queries), m_Entities(entities) {}
	public:
		inline QueryBuilder<Query> All() const { return QueryBuilder<Query>(m_Queries, m_Entities, QueryTarget::AllEntities); }
		inline QueryBuilder<Query> Deleted() const { return QueryBuilder<Query>(m_Queries, m_Entities, QueryTarget::DeletedEntities); }
		inline QueryBuilder<CreatedEntitiesQuery> Created() const { return QueryBuilder<CreatedEntitiesQuery>(m_Queries, m_Entities, QueryTarget::CreatedEntities); }
	private:
		Entities& m_Entities;
		QueryCache& m_Queries;
	};
}