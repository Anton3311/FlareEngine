#pragma once

#include "FlareECS/Entities.h"
#include "FlareECS/Entity/ComponentGroup.h"
#include "FlareECS/Commands/Command.h"

namespace Flare
{
	class FLAREECS_API AddComponentCommand : public Command
	{
	public:
		AddComponentCommand() = default;
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

	template<typename T>
	class AddComponentWithDataCommand : public Command
	{
	public:
		AddComponentWithDataCommand() = default;
		AddComponentWithDataCommand(Entity entity, const T& data)
			: m_Entity(entity), m_Data(data) {}
	public:
		virtual void Apply(World& world) override
		{
			world.AddEntityComponent<T>(m_Entity, m_Data);
		}
	private:
		Entity m_Entity;
		T m_Data;
	};

	class FLAREECS_API RemoveComponentCommand : public Command
	{
	public:
		RemoveComponentCommand() = default;
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

	template<typename... T>
	class CreateEntityWithDataCommand : public Command
	{
	public:
		CreateEntityWithDataCommand() = default;
		CreateEntityWithDataCommand(const T& ...components)
			: m_Components(components...) {}

		virtual void Apply(World& world) override
		{
			std::apply([&world](const T& ...components) { world.CreateEntity<T...>(components...); }, m_Components);
		}
	private:
		std::tuple<T...> m_Components;
	};

	class FLAREECS_API DeleteEntityCommand : public Command
	{
	public:
		DeleteEntityCommand() = default;
		DeleteEntityCommand(Entity entity)
			: m_Entity(entity) {}

		virtual void Apply(World& world) override;
	private:
		Entity m_Entity;
	};
}