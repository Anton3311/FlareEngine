#pragma once

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/EntityStorage/EntityStorage.h"

namespace Flare
{
	struct DeletedEntitiesStorage
	{
		inline void Clear()
		{
			DataStorage.Release();
			Ids.clear();
		}

		EntityStorage DataStorage;
		std::vector<Entity> Ids;
	};
}