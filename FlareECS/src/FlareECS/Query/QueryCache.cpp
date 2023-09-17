#include "QueryCache.h"

#include "FlareCore/Core.h"

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
			auto it = m_Archetypes.ComponentToArchetype.find(components[i].Masked());
			if (it != m_Archetypes.ComponentToArchetype.end())
			{
				for (std::pair<ArchetypeId, size_t> archetype : it->second)
				{
					if (matched.find(archetype.first) != matched.end())
						continue;

					if (CompareComponentSets(m_Archetypes[archetype.first].Components, query.Components))
						matched.insert(archetype.first);
				}
			}
		}

		query.MatchedArchetypes = std::move(matched);

		for (size_t i = 0; i < components.GetCount(); i++)
			m_CachedMatches[components[i].Masked()].push_back(id);

		return Query(id, m_Registry, *this);
	}

	void QueryCache::OnArchetypeCreated(ArchetypeId archetype)
	{
		const ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];

		for (ComponentId component : archetypeRecord.Components)
		{
			auto it = m_CachedMatches.find(component);
			if (it == m_CachedMatches.end())
				continue;

			const auto& queries = it->second;
			for (QueryId queryId : queries)
			{
				QueryData& query = m_Queries[queryId];

				if (CompareComponentSets(archetypeRecord.Components, query.Components))
					query.MatchedArchetypes.insert(archetype);
			}
		}
	}

	bool QueryCache::CompareComponentSets(const std::vector<ComponentId>& archetypeComponents, const std::vector<ComponentId>& queryComponents)
	{
		size_t queryComponentIndex = 0;
		size_t i = 0;
		while (i < archetypeComponents.size() && queryComponentIndex < queryComponents.size())
		{
			bool match = archetypeComponents[i].CompareMasked(queryComponents[queryComponentIndex]);
			bool without = HAS_BIT(queryComponents[queryComponentIndex].GetIndex(), (uint32_t)QueryFilterType::Without);

			if (match && without)
				return false;

			if (without)
			{
				if (archetypeComponents[i].GetIndex() > (queryComponents[queryComponentIndex].GetIndex() & ComponentId::INDEX_MASK))
				{
					queryComponentIndex++;
					continue;
				}
				else if (i == archetypeComponents.size() - 1)
					queryComponentIndex++;
				else
					continue;
			}

			if (match)
				queryComponentIndex++;

			if (queryComponentIndex == queryComponents.size())
				break;

			++i;
		}

		while (queryComponentIndex < queryComponents.size() && HAS_BIT(queryComponents[queryComponentIndex].GetIndex(), (uint32_t)QueryFilterType::Without))
			++queryComponentIndex;

		return queryComponentIndex == queryComponents.size();
	}
}