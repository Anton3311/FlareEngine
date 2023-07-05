#pragma once

#include "FlareECS/Entity/Archetype.h"

#include "FlareECS/Query/EntityView.h"
#include "FlareECS/Query/QueryCache.h"

#include <functional>

namespace Flare
{
	using SystemFunction = std::function<void(EntityView)>;
	struct System
	{
		QueryId Query;
		ArchetypeId Archetype;
		bool IsArchetypeQuery;
		SystemFunction Function;
	};
}