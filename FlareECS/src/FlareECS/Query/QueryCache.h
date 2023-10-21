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
	class FLAREECS_API Entities;
	class FLAREECS_API Query;

	class FLAREECS_API QueryCache
	{
	public:
		QueryCache(Entities& entities, Archetypes& archetypes)
			: m_Entities(entities), m_Archetypes(archetypes) {}

		~QueryCache();

		QueryCache(const QueryCache&) = delete;
		QueryCache& operator=(const QueryCache&) = delete;

		const QueryData& operator[](QueryId id) const;
	public:
		Query CreateQuery(QueryCreationData& creationData);

		void OnArchetypeCreated(ArchetypeId archetype);
	private:
		bool CompareComponentSets(const std::vector<ComponentId>& archetypeComponents, const std::vector<ComponentId>& queryComponents);
	private:
		Entities& m_Entities;
		Archetypes& m_Archetypes;

		std::vector<QueryData> m_Queries;
		std::unordered_map<ComponentId, std::vector<QueryId>> m_CachedMatches;
	};
}