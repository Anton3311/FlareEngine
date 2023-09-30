#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Commands/CommandsStorage.h"
#include "FlareECS/Commands/Command.h"
#include "FlareECS/Commands/Commands.h"

#include <type_traits>

namespace Flare
{
	class FLAREECS_API World;
	class FLAREECS_API CommandBuffer
	{
	public:
		CommandBuffer(World& world);

		template<typename T>
		void AddCommand(const T& command)
		{
			static_assert(std::is_base_of_v<Command, T> == true, "T is not a Command");
			static_assert(std::is_default_constructible_v<T> == true, "T must be have a default constructor");

			CommandMetadata metadata;
			metadata.CommandSize = sizeof(T);

			T* commandData = (T*)m_Storage.Allocate(metadata);
			FLARE_CORE_ASSERT(commandData);

			new(commandData) T;

			*commandData = command;
		}

		template<typename T>
		void AddComponent(Entity entity, ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
		{
			AddCommand<AddComponentCommand>(AddComponentCommand(entity, COMPONENT_ID(T), initStrategy));
		}

		template<typename T>
		void AddComponentWithData(Entity entity, const T& component)
		{
			AddCommand<AddComponentWithDataCommand<T>>(AddComponentWithDataCommand<T>(entity, component));
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			AddCommand<RemoveComponentCommand>(RemoveComponentCommand(entity, COMPONENT_ID(T)));
		}

		template<typename... T>
		void CreateEntity(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
		{
			AddCommand<CreateEntityCommand<T...>>(CreateEntityCommand<T...>(initStrategy));
		}

		template<typename... T>
		void CreateEntity(const T& ...components)
		{
			AddCommand<CreateEntityWithDataCommand<T...>>(CreateEntityWithDataCommand<T...>(components...));
		}

		void DeleteEntity(Entity entity);

		void Execute();
	private:
		World& m_World;
		CommandsStorage m_Storage;
	};
}