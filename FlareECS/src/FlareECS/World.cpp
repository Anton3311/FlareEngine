#include "World.h"

namespace Flare
{
	World* s_CurrentWorld = nullptr;

	World::World(ECSContext& context)
		: m_SystemsManager(*this),
		Components(context.Components),
		m_Archetypes(context.Archetypes),
		Entities(context.Components, m_Queries, context.Archetypes),
		m_Queries(Entities, context.Archetypes)
	{
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
		Entities.DeleteEntity(entity);
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		return Entities.IsEntityAlive(entity);
	}

	const std::vector<ComponentId>& World::GetEntityComponents(Entity entity)
	{
		return Entities.GetEntityComponents(entity);
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