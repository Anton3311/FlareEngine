#pragma once

#include "FlareECS/Entity/Archetypes.h"
#include "FlareECS/Entity/Components.h"

namespace Flare
{
	struct ECSContext
	{
		inline void Clear()
		{
			Archetypes.Clear();
			Components.Clear();
		}

		Flare::Archetypes Archetypes;
		Flare::Components Components;
	};
}