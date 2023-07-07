#pragma once

#include "FlareECS/Entity/Archetype.h"

#include "FlareECS/Query/EntityView.h"
#include "FlareECS/Query/QueryCache.h"
#include "FlareECS/Query/Query.h"

#include <functional>

namespace Flare
{
	using SystemFunction = std::function<void(EntityView)>;
	struct System
	{
		System(const Query& query, const SystemFunction& systemFunction)
			: SystemQuery(query), Archetype(INVALID_ARCHETYPE_ID), IsArchetypeQuery(false), Function(systemFunction) {}

		Query SystemQuery;
		ArchetypeId Archetype;
		bool IsArchetypeQuery;
		SystemFunction Function;
	};
}