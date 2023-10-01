#include "Commands.h"

#include "FlareECS/World.h"

namespace Flare
{
	AddComponentCommand::AddComponentCommand(FutureEntity entity, ComponentId component, ComponentInitializationStrategy initStrategy)
		: m_Entity(entity), m_Component(component), m_InitStrategy(initStrategy) {}
	
	void AddComponentCommand::Apply(CommandContext& context, World& world)
	{
		FLARE_CORE_ASSERT(world.IsEntityAlive(context.GetEntity(m_Entity)));
		world.Entities.AddEntityComponent(context.GetEntity(m_Entity), m_Component, nullptr, m_InitStrategy);
	}

	RemoveComponentCommand::RemoveComponentCommand(FutureEntity entity, ComponentId component)
		: m_Entity(entity), m_Component(component) {}

	void RemoveComponentCommand::Apply(CommandContext& context, World& world)
	{
		FLARE_CORE_ASSERT(world.IsEntityAlive(context.GetEntity(m_Entity)));
		world.Entities.RemoveEntityComponent(context.GetEntity(m_Entity), m_Component);
	}

	void DeleteEntityCommand::Apply(CommandContext& context, World& world)
	{
		world.DeleteEntity(m_Entity);
	}
}
