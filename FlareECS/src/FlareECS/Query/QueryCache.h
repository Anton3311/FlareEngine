#pragma once

#include "Flare/Core/Assert.h"

#include "FlareECS/Component.h"
#include "FlareECS/Archetype.h"

#include "FlareECS/Query/QueryData.h"

#include <vector>
#include <unordered_map>

namespace Flare
{
	class Registry;

	using QueryId = size_t;
	class QueryCache
	{
	public:
		QueryCache(Registry& registry)
			: m_Registry(registry) {}
	public:
		QueryId AddQuery(const ComponentSet& components);
		void OnArchetypeCreated(ArchetypeId archetype);

		inline const QueryData& operator[](QueryId id) const
		{
			FLARE_CORE_ASSERT(id < m_Queries.size());
			return m_Queries[id];
		}
	private:
		bool CompareComponentSets(const ComponentSet& archetypeComponents, const ComponentSet& queryComponents);
	private:
		Registry& m_Registry;

		std::vector<QueryData> m_Queries;
		std::unordered_map<ComponentId, std::vector<QueryId>> m_CachedMatches;
	};
}