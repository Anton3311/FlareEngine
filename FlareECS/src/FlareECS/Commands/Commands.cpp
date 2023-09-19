#include "Commands.h"

#include "FlareECS/World.h"

namespace Flare
{
	AddComponentCommand::AddComponentCommand(Entity entity, ComponentId component, ComponentInitializationStrategy initStrategy)
		: m_Entity(entity), m_Component(component), m_InitStrategy(initStrategy) {}
	
	void AddComponentCommand::Apply(World& world)
	{
		world.Entities.AddEntityComponent(m_Entity, m_Component, nullptr, m_InitStrategy);
	}

	RemoveComponentCommand::RemoveComponentCommand(Entity entity, ComponentId component)
		: m_Entity(entity), m_Component(component) {}

	void RemoveComponentCommand::Apply(World& world)
	{
		world.Entities.RemoveEntityComponent(m_Entity, m_Component);
	}

	void DeleteEntityCommand::Apply(World& world)
	{
		world.DeleteEntity(m_Entity);
	}
}
