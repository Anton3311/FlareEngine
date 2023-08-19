#include "Commands.h"

namespace Flare
{
	AddComponentCommand::AddComponentCommand(Entity entity, ComponentId component, ComponentInitializationStrategy initStrategy)
		: m_Entity(entity), m_Component(component), m_InitStrategy(initStrategy) {}
	
	void AddComponentCommand::Apply(World& world)
	{
		world.GetRegistry().AddEntityComponent(m_Entity, m_Component, nullptr, m_InitStrategy);
	}

	RemoveComponentCommand::RemoveComponentCommand(Entity entity, ComponentId component)
		: m_Entity(entity), m_Component(component) {}

	void RemoveComponentCommand::Apply(World& world)
	{
		world.GetRegistry().RemoveEntityComponent(m_Entity, m_Component);
	}
}
