#include "QueryCache.h"

#include "FlareECS/Registry.h"
#include "FlareECS/Query/Query.h"

#include <unordered_set>
#include <algorithm>

namespace Flare
{
	const QueryData& QueryCache::operator[](QueryId id) const
	{
		FLARE_CORE_ASSERT(id < m_Queries.size());
		return m_Queries[id];
	}

	Query QueryCache::AddQuery(const ComponentSet& components)
	{
		std::unordered_set<ArchetypeId> matched;

		QueryId id = m_Queries.size();
		QueryData& query = m_Queries.emplace_back();
		query.Id = id;
		query.Components.resize(components.GetCount());

		std::memcpy(query.Components.data(), components.GetIds(), components.GetCount() * sizeof(ComponentId));
		std::sort(query.Components.begin(), query.Components.end());

		for (size_t i = 0; i < components.GetCount(); i++)
		{
			auto it = m_Registry.m_ComponentToArchetype.find(components[i]);
			if (it != m_Registry.m_ComponentToArchetype.end())
			{
				for (std::pair<ArchetypeId, size_t> archetype : it->second)
				{
					if (matched.find(archetype.first) != matched.end())
						continue;

					if (CompareComponentSets(m_Registry.GetArchetypeRecord(archetype.first).Data.Components, query.Components))
						matched.insert(archetype.first);
				}
			}
		}

		query.MatchedArchetypes = std::move(matched);

		for (size_t i = 0; i < components.GetCount(); i++)
			m_CachedMatches[components[i]].push_back(id);

		return Query(id, m_Registry, *this);
	}

	void QueryCache::OnArchetypeCreated(ArchetypeId archetype)
	{
		ArchetypeRecord& archetypeRecord = m_Registry.GetArchetypeRecord(archetype);

		for (ComponentId component : archetypeRecord.Data.Components)
		{
			auto it = m_CachedMatches.find(component);
			if (it == m_CachedMatches.end())
				continue;

			const auto& queries = it->second;
			for (QueryId queryId : queries)
			{
				QueryData& query = m_Queries[queryId];

				if (CompareComponentSets(archetypeRecord.Data.Components, query.Components))
					query.MatchedArchetypes.insert(archetype);
			}
		}
	}

	bool QueryCache::CompareComponentSets(const std::vector<ComponentId>& archetypeComponents, const std::vector<ComponentId>& queryComponents)
	{
		// Query components must be contained by archetype components
		if (queryComponents.size() > archetypeComponents.size())
			return false;

		size_t queryComponentIndex = 0;
		for (size_t i = 0; i < archetypeComponents.size(); i++)
		{
			if (archetypeComponents[i] == queryComponents[queryComponentIndex])
				queryComponentIndex++;

			if (queryComponentIndex == queryComponents.size())
				break;
		}

		return queryComponentIndex == queryComponents.size();
	}
}