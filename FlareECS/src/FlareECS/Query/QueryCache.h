#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Archetype.h"
#include "FlareECS/Entity/Archetypes.h"

#include "FlareECS/Query/QueryData.h"

#include <vector>
#include <unordered_map>

namespace Flare
{
	class Entities;
	class Query;

	class FLAREECS_API QueryCache : public ArchetypeUpdateHandler
	{
	public:
		QueryCache(Archetypes& archetypes);
		~QueryCache();

		QueryCache(const QueryCache&) = delete;
		QueryCache& operator=(const QueryCache&) = delete;

		void Clear();

		const QueryData& operator[](QueryId id) const;
		inline const QueryData& GetQueryData(QueryId id) const { return operator[](id); }

		QueryId CreateQuery(QueryCreationData& creationData);

		void OnArchetypeCreated(ArchetypeId archetype) override;
	private:
		bool CompareComponentSets(const std::vector<ComponentId>& archetypeComponents, const std::vector<ComponentId>& queryComponents);
	private:
		Archetypes& m_Archetypes;

		std::vector<QueryData> m_Queries;
		std::unordered_map<ComponentId, std::vector<QueryId>> m_CachedMatches;
	};
}