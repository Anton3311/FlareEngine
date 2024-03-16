#include "QueryCache.h"

#include "FlareCore/Core.h"

#include "FlareECS/Entities.h"
#include "FlareECS/Query/Query.h"

#include <unordered_set>
#include <algorithm>

namespace Flare
{
	QueryCache::~QueryCache()
	{
		for (const QueryData& query : m_Queries)
		{
			if (query.Target == QueryTarget::DeletedEntities)
			{
				for (ArchetypeId archetype : query.MatchedArchetypes)
					m_Archetypes.Records[archetype].DeletionQueryReferences--;
			}
			else if (query.Target == QueryTarget::CreatedEntities)
			{
				for (ArchetypeId archetype : query.MatchedArchetypes)
					m_Archetypes.Records[archetype].CreatedEntitiesQueryReferences--;
			}
		}
	}

	const QueryData& QueryCache::operator[](QueryId id) const
	{
		FLARE_CORE_ASSERT(id < m_Queries.size());
		return m_Queries[id];
	}

	QueryId QueryCache::CreateQuery(QueryCreationData& creationData)
	{
		QueryId id = m_Queries.size();
		QueryData& query = m_Queries.emplace_back();
		query.Id = id;
		query.Target = creationData.Target;
		query.Components = std::move(creationData.Components);
		query.MatchedArchetypes;

		std::sort(query.Components.begin(), query.Components.end());

		for (size_t i = 0; i < query.Components.size(); i++)
		{
			auto it = m_Archetypes.ComponentToArchetype.find(query.Components[i].Masked());
			if (it == m_Archetypes.ComponentToArchetype.end())
				continue;

			for (std::pair<ArchetypeId, size_t> archetype : it->second)
			{
				if (query.MatchedArchetypes.find(archetype.first) != query.MatchedArchetypes.end())
					continue;

				if (!CompareComponentSets(m_Archetypes[archetype.first].Components, query.Components))
					continue;

				query.MatchedArchetypes.insert(archetype.first);

				if (query.Target == QueryTarget::DeletedEntities)
					m_Archetypes.Records[archetype.first].DeletionQueryReferences++;
				else if (query.Target == QueryTarget::CreatedEntities)
					m_Archetypes.Records[archetype.first].CreatedEntitiesQueryReferences += 1;
			}
		}

		for (size_t i = 0; i < query.Components.size(); i++)
			m_CachedMatches[query.Components[i].Masked()].push_back(id);

		return id;
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

				if (query.MatchedArchetypes.find(archetype) != query.MatchedArchetypes.end())
					continue;

				if (CompareComponentSets(archetypeRecord.Components, query.Components))
				{
					query.MatchedArchetypes.insert(archetype);

					if (query.Target == QueryTarget::DeletedEntities)
						m_Archetypes.Records[archetype].DeletionQueryReferences++;
					else if (query.Target == QueryTarget::CreatedEntities)
						m_Archetypes.Records[archetype].CreatedEntitiesQueryReferences += 1;
				}
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