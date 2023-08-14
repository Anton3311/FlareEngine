#include "World.h"

namespace Flare
{
	World* s_CurrentWorld = nullptr;

	World::World()
	{
		FLARE_CORE_ASSERT(s_CurrentWorld == nullptr, "Multiple ECS Worlds");
		s_CurrentWorld = this;
	}

	World::~World()
	{
		s_CurrentWorld = nullptr;
	}

	void World::DeleteEntity(Entity entity)
	{
		m_Registry.DeleteEntity(entity);
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		return m_Registry.IsEntityAlive(entity);
	}

	const std::vector<ComponentId>& World::GetEntityComponents(Entity entity)
	{
		return m_Registry.GetEntityComponents(entity);
	}

	Entity World::GetSingletonEntity(const Query& query)
	{
		return m_Registry.GetSingletonEntity(query).value_or(Entity());
	}

	World& World::GetCurrent()
	{
		FLARE_CORE_ASSERT(s_CurrentWorld != nullptr);
		return *s_CurrentWorld;
	}

	Registry& World::GetRegistry()
	{
		return m_Registry;
	}

	SystemsManager& World::GetSystemsManager()
	{
		return m_SystemsManager;
	}

	const SystemsManager& World::GetSystemsManager() const
	{
		return m_SystemsManager;
	}
}