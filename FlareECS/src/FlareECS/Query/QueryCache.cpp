#include "QueryCache.h"

#include "FlareCore/Core.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entities.h"
#include "FlareECS/Query/Query.h"

#include <unordered_set>
#include <algorithm>

namespace Flare
{
	QueryCache::QueryCache(Archetypes& archetypes)
			: m_Archetypes(archetypes)
	{
		m_Archetypes.AddUpdateHandler(this);
	}

	QueryCache::~QueryCache()
	{
		FLARE_PROFILE_FUNCTION();
		m_Archetypes.RemoveUpdateHandler(this);

		Clear();
	}

	void QueryCache::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		for (const QueryData& query : m_Queries)
		{
			if (query.Target == QueryTarget::DeletedEntities)
			{
				for (ArchetypeId archetype : query.MatchingArchetypes)
					m_Archetypes.GetMutableRecord(archetype).DeletionQueryReferences--;
			}
			else if (query.Target == QueryTarget::CreatedEntities)
			{
				for (ArchetypeId archetype : query.MatchingArchetypes)
					m_Archetypes.GetMutableRecord(archetype).CreatedEntitiesQueryReferences--;
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
		FLARE_PROFILE_FUNCTION();

		QueryId id = m_Queries.size();
		QueryData& query = m_Queries.emplace_back();
		query.Id = id;
		query.Target = creationData.Target;
		query.Components = std::move(creationData.Components);
		query.MatchingArchetypes;

		std::sort(query.Components.begin(),
			query.Components.end(),
			[](const QueryData::ComponentEntry& a, const QueryData::ComponentEntry& b) -> bool
			{
				return a.Id < b.Id;
			});

		for (size_t i = 0; i < query.Components.size(); i++)
		{
			auto archetypes = m_Archetypes.GetArchetypesWithComponent(query.Components[i].Id);
			if (!archetypes)
				continue;

			for (std::pair<ArchetypeId, size_t> archetype : *archetypes)
			{
				if (query.MatchingArchetypes.find(archetype.first) != query.MatchingArchetypes.end())
					continue;

				const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(archetype.first);
				if (!CompareComponentSets(archetypeComponents.GetComponentsAsSpan(), Span<QueryData::ComponentEntry>::FromVector(query.Components)))
					continue;

				query.MatchingArchetypes.insert(archetype.first);

				if (query.Target == QueryTarget::DeletedEntities)
					m_Archetypes.GetMutableRecord(archetype.first).DeletionQueryReferences++;
				else if (query.Target == QueryTarget::CreatedEntities)
					m_Archetypes.GetMutableRecord(archetype.first).CreatedEntitiesQueryReferences += 1;
			}
		}

		for (size_t i = 0; i < query.Components.size(); i++)
			m_CachedMatches[query.Components[i].Id].push_back(id);

		return id;
	}

	void QueryCache::OnArchetypeCreated(ArchetypeId archetype)
	{
		FLARE_PROFILE_FUNCTION();
		const ArchetypeRecord& archetypeRecord = m_Archetypes[archetype];
		const ArchetypeComponents& archetypeComponents = m_Archetypes.GetArchetypeComponents(archetype);

		for (ComponentId component : archetypeComponents.GetComponentsAsSpan())
		{
			auto it = m_CachedMatches.find(component);
			if (it == m_CachedMatches.end())
				continue;

			const auto& queries = it->second;
			for (QueryId queryId : queries)
			{
				QueryData& query = m_Queries[queryId];

				if (query.MatchingArchetypes.find(archetype) != query.MatchingArchetypes.end())
					continue;

				if (CompareComponentSets(archetypeComponents.GetComponentsAsSpan(), Span(query.Components.data(), query.Components.size())))
				{
					query.MatchingArchetypes.insert(archetype);

					if (query.Target == QueryTarget::DeletedEntities)
						m_Archetypes.GetMutableRecord(archetype).DeletionQueryReferences++;
					else if (query.Target == QueryTarget::CreatedEntities)
						m_Archetypes.GetMutableRecord(archetype).CreatedEntitiesQueryReferences++;
				}
			}
		}
	}

	void QueryCache::OnArchetypeRemoved(ArchetypeId id)
	{
		FLARE_PROFILE_FUNCTION();

		for (QueryData& data : m_Queries)
		{
			auto it = data.MatchingArchetypes.find(id);
			if (it != data.MatchingArchetypes.end())
			{
				data.MatchingArchetypes.erase(it);
			}
		}
	}

	bool QueryCache::CompareComponentSets(Span<const ComponentId> archetypeComponents, Span<const QueryData::ComponentEntry> queryComponents)
	{
		FLARE_PROFILE_FUNCTION();

		for (const QueryData::ComponentEntry& entry : queryComponents)
		{
			bool contains = archetypeComponents.Contains(entry.Id);
			
			switch (entry.Filter)
			{
			case QueryFilterType::With:
				if (!contains)
					return false;
				break;
			case QueryFilterType::Without:
				if (contains)
					return false;
				break;
			default:
				FLARE_VERIFY_UNREACHABLE();
			}
		}

		return true;
	}
}