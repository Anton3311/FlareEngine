#pragma once

#include "FlareECS/Registry.h"
#include "FlareECS/Entity/ComponentGroup.h"
#include "FlareECS/Commands/Command.h"

namespace Flare
{
	class FLAREECS_API AddComponentCommand : public Command
	{
	public:
		AddComponentCommand(Entity entity,
			ComponentId component,
			ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor);
	public:
		virtual void Apply(World& world) override;
	private:
		ComponentId m_Component;
		Entity m_Entity;
		ComponentInitializationStrategy m_InitStrategy;
	};

	class FLAREECS_API RemoveComponentCommand : public Command
	{
	public:
		RemoveComponentCommand(Entity entity, ComponentId component);

		virtual void Apply(World& world) override;
	private:
		Entity m_Entity;
		ComponentId m_Component;
	};

	template<typename... T>
	class CreateEntityCommand : public Command
	{
	public:
		CreateEntityCommand(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
			: m_InitStrategy(initStrategy) {}

		virtual void Apply(World& world) override
		{
			world.CreateEntity<T...>(m_InitStrategy);
		}
	private:
		ComponentInitializationStrategy m_InitStrategy;
	};

	class FLAREECS_API DeleteEntityCommand : public Command
	{
	public:
		DeleteEntityCommand(Entity entity)
			: m_Entity(entity) {}

		virtual void Apply(World& world) override;
	private:
		Entity m_Entity;
	};
}