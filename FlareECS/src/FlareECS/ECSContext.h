#pragma once

#include "FlareECS/Entity/Archetypes.h"
#include "FlareECS/Entity/Components.h"
#include "FlareECS/System/SystemsRegistry.h"
#include "FlareECS/Query/QueryCache.h"

namespace Flare
{
	struct FLAREECS_API ECSContext
	{
	public:
		ECSContext();
		~ECSContext();

		void Clear();

		static ECSContext& GetGlobal();
	public:
		Flare::Archetypes Archetypes;
		Flare::Components Components;
		Flare::SystemsRegistry SystemsRegistry;
		QueryCache Queries;
	};
}