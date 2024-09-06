#include "Query.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	std::optional<Entity> Query::TryGetFirstEntityId() const
	{
		FLARE_PROFILE_FUNCTION();
		for (ArchetypeId archetype : GetMatchingArchetypes())
		{
			const EntityStorage& storage = m_Entities->GetEntityStorage(archetype);
			if (storage.GetEntityCount() == 0)
				continue;

			return storage.GetEntityId(0);
		}

		return {};
	}

	size_t Query::GetEntitiesCount() const
	{
		FLARE_PROFILE_FUNCTION();
		size_t count = 0;

		switch (m_Queries->GetQueryData(m_Id).Target)
		{
		case QueryTarget::AllEntities:
			for (ArchetypeId archetype : GetMatchingArchetypes())
				count += m_Entities->GetEntityStorage(archetype).GetEntityCount();
			break;
		case QueryTarget::DeletedEntities:
			for (ArchetypeId archetype : GetMatchingArchetypes())
				count += m_Entities->GetDeletedEntityStorage(archetype).GetEntityCount();
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}
		
		return count;
	}



	std::optional<Entity> CreatedEntitiesQuery::TryGetFirstEntityId() const
	{
		FLARE_PROFILE_FUNCTION();
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
		FLARE_PROFILE_FUNCTION();

		size_t count = 0;
		for (ArchetypeId archetype : GetMatchingArchetypes())
		{
			Span<Entity> ids = m_Entities->GetCreatedEntities(archetype);
			count += ids.GetSize();
		}

		return count;
	}
}