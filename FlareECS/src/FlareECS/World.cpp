#include "World.h"

namespace Flare
{
	void World::RegisterSystem(const Query& query, const SystemFunction& system)
	{
		m_Systems.emplace_back(query, system);
	}

	void World::OnUpdate()
	{
		for (const System& system : m_Systems)
		{
			if (system.IsArchetypeQuery)
			{
				FLARE_CORE_ASSERT(false, "Not implemented");
			}
			else
			{
				for (EntityView view : system.SystemQuery)
					system.Function(view);
			}
		}
	}
}