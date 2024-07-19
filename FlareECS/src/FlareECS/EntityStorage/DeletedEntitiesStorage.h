#pragma once

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/EntityStorage/EntityStorage.h"

namespace Flare
{
	struct DeletedEntitiesStorage
	{
		inline void Clear()
		{
			DataStorage.Clear();
			Ids.clear();
		}

		EntityDataStorage DataStorage;
		std::vector<Entity> Ids;
	};
}