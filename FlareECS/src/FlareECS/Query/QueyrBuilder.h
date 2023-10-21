#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Query/QueryCache.h"
#include "FlareECS/Query/QueryFilters.h"
#include "FlareECS/Query/Query.h"

namespace Flare
{
	class FLAREECS_API QueryBuilder
	{
	public:
		QueryBuilder(QueryCache& queries);

		template<typename... T>
		QueryBuilder& With()
		{
			size_t count = sizeof...(T);
			m_Ids.reserve(m_Ids.size() + count);

			size_t index = 0;

			([&]
			{
				m_Ids.push_back(COMPONENT_ID(T));
			} (), ...);

			return *this;
		}

		template<typename T>
		QueryBuilder& Without()
		{
			size_t count = sizeof...(T);
			m_Ids.reserve(m_Ids.size() + count);

			size_t index = 0;

			([&]
			{
				ComponentId id = COMPONENT_ID(T);
				m_Ids.push_back(ComponentId(
					id.GetIndex() | (uint32_t)QueryFilterType::Without, 
					id.GetGeneration()));
			} (), ...);

			return *this;
		}

		Query Create();
	private:
		QueryCache& m_Queries;
		std::vector<ComponentId> m_Ids;
	};
}