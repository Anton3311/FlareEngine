#include "QueryCache.h"

#include "FlareECS/Registry.h"

#include <unordered_set>

namespace Flare
{
	QueryId QueryCache::AddQuery(const ComponentSet& components)
	{
		std::unordered_set<ArchetypeId> matched;

		for (size_t i = 0; i < components.GetCount(); i++)
		{
			auto it = m_Registry.m_ComponentToArchetype.find(components[i]);
			if (it != m_Registry.m_ComponentToArchetype.end())
			{
				for (std::pair<ArchetypeId, size_t> archetype : it->second)
				{
					if (matched.find(archetype.first) != matched.end())
						continue;

					if (CompareComponentSets(m_Registry.GetArchetypeRecord(archetype.first).Data.Components, components))
						matched.insert(archetype.first);
				}
			}
		}

		QueryId id = m_Queries.size();
		QueryData& query = m_Queries.emplace_back();

		query.Components.resize(components.GetCount());
		for (size_t i = 0; i < components.GetCount(); i++)
		{
			query.Components[i] = components[i];
			m_CachedMatches[components[i]].push_back(id);
		}

		query.MatchedArchetypes.reserve(matched.size());
		for (ArchetypeId archetype : matched)
			query.MatchedArchetypes.insert(archetype);

		return id;
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
				QueryData& data = m_Queries[queryId];

				if (CompareComponentSets(archetypeRecord.Data.Components, data.Components))
					data.MatchedArchetypes.insert(archetype);
			}
		}
	}

	bool QueryCache::CompareComponentSets(const ComponentSet& archetypeComponents, const ComponentSet& queryComponents)
	{
		// Query components must be contained by archetype components
		if (queryComponents.GetCount() > archetypeComponents.GetCount())
			return false;

		size_t queryComponentIndex = 0;
		for (size_t i = 0; i < archetypeComponents.GetCount(); i++)
		{
			if (archetypeComponents[i] == queryComponents[queryComponentIndex])
				queryComponentIndex++;

			if (queryComponentIndex == queryComponents.GetCount())
				break;
		}

		return queryComponentIndex == queryComponents.GetCount();
	}
}