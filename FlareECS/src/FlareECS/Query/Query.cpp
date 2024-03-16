#include "Query.h"

namespace Flare
{
	QueryIterator Query::begin() const
	{
		FLARE_CORE_ASSERT(m_Entities);
		FLARE_CORE_ASSERT(m_Queries);

		const auto& data = (*m_Queries)[m_Id];
		return QueryIterator(*m_Entities, data.Target, data.MatchedArchetypes.begin());
	}

	QueryIterator Query::end() const
	{
		FLARE_CORE_ASSERT(m_Entities);
		FLARE_CORE_ASSERT(m_Queries);
		const auto& data = (*m_Queries)[m_Id];
		return QueryIterator(*m_Entities, data.Target, data.MatchedArchetypes.end());
	}

	std::optional<Entity> Query::TryGetFirstEntityId() const
	{
		for (ArchetypeId archetype : GetMatchingArchetypes())
		{
			const EntityStorage& storage = m_Entities->GetEntityStorage(archetype);
			if (storage.GetEntitiesCount() == 0)
				continue;

			uint32_t firstEntityIndex = storage.GetEntityIndices()[0];
			std::optional<Entity> entity = m_Entities->FindEntityByIndex(firstEntityIndex);

			FLARE_CORE_ASSERT(entity);
			return *entity;
		}

		return {};
	}

	size_t Query::GetEntitiesCount() const
	{
		size_t count = 0;
		for (ArchetypeId archetype : GetMatchingArchetypes())
			count += m_Entities->GetEntityStorage(archetype).GetEntitiesCount();
		
		return count;
	}



	std::optional<Entity> CreatedEntitiesQuery::TryGetFirstEntityId() const
	{
		const QueryData& queryData = (*m_Queries)[m_Id];
		for (ArchetypeId archetype : queryData.MatchedArchetypes)
		{
			Span<Entity> ids = m_Entities->GetCreatedEntities(archetype);
			if (ids.GetSize() == 0)
				continue;

			return ids[0];
		}

		return {};
	}

	size_t CreatedEntitiesQuery::GetEntitiesCount() const
	{
		size_t count = 0;
		for (ArchetypeId archetype : GetMatchingArchetypes())
		{
			Span<Entity> ids = m_Entities->GetCreatedEntities(archetype);
			count += ids.GetSize();
		}

		return count;
	}
}