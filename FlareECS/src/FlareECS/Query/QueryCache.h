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
	class FLAREECS_API Registry;
	class FLAREECS_API Query;

	class FLAREECS_API QueryCache
	{
	public:
		QueryCache(Registry& registry, const Archetypes& archetypes)
			: m_Registry(registry), m_Archetypes(archetypes) {}

		const QueryData& operator[](QueryId id) const;
	public:
		Query AddQuery(const ComponentSet& components);
		void OnArchetypeCreated(ArchetypeId archetype);
	private:
		bool CompareComponentSets(const std::vector<ComponentId>& archetypeComponents, const std::vector<ComponentId>& queryComponents);
	private:
		Registry& m_Registry;
		const Archetypes& m_Archetypes;

		std::vector<QueryData> m_Queries;
		std::unordered_map<ComponentId, std::vector<QueryId>> m_CachedMatches;
	};
}