#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Commands/CommandsStorage.h"
#include "FlareECS/Commands/Command.h"
#include "FlareECS/Commands/Commands.h"

#include <type_traits>

namespace Flare
{
	class FLAREECS_API CommandBuffer;
	class FutureEntityCommands
	{
	public:
		constexpr FutureEntityCommands(FutureEntity entity, CommandBuffer& commandBuffer)
			: m_FutureEntity(entity), m_CommandBuffer(commandBuffer) {}
	public:
		template<typename T>
		FutureEntityCommands& AddComponent(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
		{
			m_CommandBuffer.AddCommand<AddComponentCommand>(AddComponentCommand(m_FutureEntity, COMPONENT_ID(T), initStrategy));
			return *this;
		}

		template<typename T>
		FutureEntityCommands& AddComponentWithData(const T& component)
		{
			m_CommandBuffer.AddCommand<AddComponentWithDataCommand<T>>(AddComponentWithDataCommand<T>(m_FutureEntity, component));
			return *this;
		}

		template<typename T>
		FutureEntityCommands& RemoveComponent()
		{
			m_CommandBuffer.AddCommand<RemoveComponentCommand>(RemoveComponentCommand(m_FutureEntity, COMPONENT_ID(T)));
			return *this;
		}
	private:
		FutureEntity m_FutureEntity;
		CommandBuffer& m_CommandBuffer;
	};

	class FLAREECS_API World;
	class FLAREECS_API CommandBuffer
	{
	public:
		CommandBuffer(World& world);

		template<typename T>
		void AddCommand(const T& command)
		{
			static_assert(std::is_base_of_v<Command, T> == true, "T is not a Command");
			static_assert(std::is_default_constructible_v<T> == true, "T must have a default constructor");

			auto [meta, commandData] = m_Storage.AllocateCommand(sizeof(T));
			FLARE_CORE_ASSERT(commandData);

			meta->CommandSize = sizeof(T);

			new(commandData) T;
			*(T*)commandData = command;
		}

		template<typename T>
		FutureEntityCommands AddEntityCommand(const T& command)
		{
			static_assert(std::is_base_of_v<EntityCommand, T> == true, "T is not an EntityCommand");

			auto [commandData, meta] = CreateCommand<T>();
			FutureEntity entity = AllocateFutureEntity(*meta);

			new(commandData) T;
			*commandData = command;

			((EntityCommand*)commandData)->Initialize(entity);
			return FutureEntityCommands(entity, *this);
		}

		template<typename... T>
		FutureEntityCommands CreateEntity(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
		{
			return AddEntityCommand(CreateEntityCommand<T...>(initStrategy));
		}

		template<typename... T>
		FutureEntityCommands CreateEntity(const T& ...components)
		{
			return AddEntityCommand(CreateEntityWithDataCommand<T...>(components...));
		}

		FutureEntityCommands GetEntity(Entity entity);

		void DeleteEntity(Entity entity);
		void Execute();
	private:
		template<typename T>
		std::pair<T*, CommandMetadata*> CreateCommand()
		{
			static_assert(std::is_base_of_v<Command, T> == true, "T is not a Command");
			static_assert(std::is_default_constructible_v<T> == true, "T must have a default constructor");

			auto [meta, commandData] = m_Storage.AllocateCommand(sizeof(T));
			FLARE_CORE_ASSERT(commandData);

			meta->CommandSize = sizeof(T);

			return { (T*)commandData, meta };
		}

		FutureEntity AllocateFutureEntity(CommandMetadata& meta)
		{
			std::optional<size_t> futureEntityLocation = m_Storage.Allocate(sizeof(Entity));
			FLARE_CORE_ASSERT(futureEntityLocation.has_value());

			meta.CommandSize += sizeof(Entity);
			return FutureEntity(futureEntityLocation.value());
		}
	private:
		World& m_World;
		CommandsStorage m_Storage;
	};
}