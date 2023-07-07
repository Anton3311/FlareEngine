#pragma once

#include "Flare/Core/Assert.h"

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Archetype.h"

#include "FlareECS/Query/QueryData.h"

#include <vector>
#include <unordered_map>

namespace Flare
{
	class Registry;

	class Query;
	class QueryCache
	{
	public:
		QueryCache(Registry& registry)
			: m_Registry(registry) {}

		const QueryData& operator[](QueryId id) const;
	public:
		Query AddQuery(const ComponentSet& components);
		void OnArchetypeCreated(ArchetypeId archetype);
	private:
		bool CompareComponentSets(const std::vector<ComponentId>& archetypeComponents, const std::vector<ComponentId>& queryComponents);
	private:
		Registry& m_Registry;

		std::vector<QueryData> m_Queries;
		std::unordered_map<ComponentId, std::vector<QueryId>> m_CachedMatches;
	};
}