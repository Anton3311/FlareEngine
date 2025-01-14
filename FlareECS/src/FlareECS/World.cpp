#include "World.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	World* s_CurrentWorld = nullptr;

	World::World(ECSContext& context)
		: Components(context.Components),
		m_Queries(context.Queries),
		m_Archetypes(context.Archetypes),
		Events(Components),
		Entities(context.Components, context.Archetypes)
	{
		FLARE_PROFILE_FUNCTION();
	}

	World::~World()
	{
		if (s_CurrentWorld == this)
			s_CurrentWorld = nullptr;
	}

	void World::MakeCurrent()
	{
		s_CurrentWorld = this;
	}

	void World::DeleteEntity(Entity entity)
	{
		Entities.DeleteEntity(entity, false);
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		return Entities.IsEntityAlive(entity);
	}

	Entity World::GetSingletonEntity(const Query& query)
	{
		return Entities.GetSingletonEntity(query).value_or(Entity());
	}

	World& World::GetCurrent()
	{
		FLARE_CORE_ASSERT(s_CurrentWorld != nullptr);
		return *s_CurrentWorld;
	}
}