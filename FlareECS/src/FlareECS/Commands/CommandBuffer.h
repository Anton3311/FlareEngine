#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Commands/CommandsStorage.h"
#include "FlareECS/Commands/Command.h"
#include "FlareECS/Commands/Commands.h"

#include <type_traits>

namespace Flare
{
	class FLAREECS_API EntitiesCommandBuffer;
	class FutureEntityCommands
	{
	public:
		constexpr FutureEntityCommands(FutureEntity entity, EntitiesCommandBuffer& commandBuffer)
			: m_FutureEntity(entity), m_CommandBuffer(commandBuffer) {}
	public:
		template<typename T>
		FutureEntityCommands& AddComponent(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor);

		template<typename T>
		FutureEntityCommands& AddComponentWithData(const T& component);

		template<typename T>
		FutureEntityCommands& SetComponent(const T& component);

		template<typename T>
		FutureEntityCommands& RemoveComponent();

		template<typename F>
		FutureEntityCommands& ExecuteFunction(F&& function);

		template<typename T>
		FutureEntityCommands& AddCommand(T&& command);

		inline FutureEntity GetFutureEntity() const { return m_FutureEntity; }
	private:
		FutureEntity m_FutureEntity;
		EntitiesCommandBuffer& m_CommandBuffer;
	};

	class FLAREECS_API World;
	class FLAREECS_API EntitiesCommandBuffer
	{
	public:
		EntitiesCommandBuffer();

		template<typename F>
		void ExecuteFunction(F&& function)
		{
			AddCommand(FunctionExecutionCommand<F, false>(std::move(function), FutureEntity()));
		}

		template<typename T>
		void AddCommand(T&& command)
		{
			static_assert(std::is_base_of_v<Command, T> == true, "T is not a Command");
			static_assert(std::is_move_constructible_v<T> == true);

			std::optional<CommandAllocation> commandAllocation = m_Storage.AllocateCommand(sizeof(T));
			FLARE_CORE_ASSERT(commandAllocation.has_value());

			T* commandData = m_Storage.Read<T>(commandAllocation.value().CommandLocation).value_or(nullptr);
			CommandMetadata* meta = m_Storage.Read<CommandMetadata>(commandAllocation.value().MetaLocation).value_or(nullptr);

			meta->CommandSize = sizeof(T);

			new(commandData) T(std::move(command));
		}

		template<typename T>
		FutureEntityCommands AddEntityCommand(T&& command)
		{
			static_assert(std::is_move_constructible_v<T> == true);
			static_assert(std::is_base_of_v<Command, T> == true, "T is not a Command");
			static_assert(std::is_base_of_v<EntityCommand, T> == true, "T is not an EntityCommand");

			std::optional<CommandAllocation> commandAllocation = m_Storage.AllocateCommand(sizeof(T));
			std::optional<size_t> entityLocation = m_Storage.Allocate(sizeof(Entity));

			FLARE_CORE_ASSERT(commandAllocation.has_value());
			FLARE_CORE_ASSERT(entityLocation.has_value());

			FutureEntity entity = FutureEntity(entityLocation.value());

			m_Storage.Write<Entity>(entityLocation.value(), Entity());

			T* commandData = m_Storage.Read<T>(commandAllocation.value().CommandLocation).value_or(nullptr);
			CommandMetadata* meta = m_Storage.Read<CommandMetadata>(commandAllocation.value().MetaLocation).value_or(nullptr);

			meta->CommandSize = sizeof(T) + sizeof(Entity);

			new(commandData) T(std::move(command));

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

		// Returns number of commands executed.
		size_t Execute(World& world);
	private:
		CommandsStorage m_Storage;
	};

	template<typename T>
	inline FutureEntityCommands& FutureEntityCommands::AddComponent(ComponentInitializationStrategy initStrategy)
	{
		m_CommandBuffer.AddCommand<AddComponentCommand>(AddComponentCommand(m_FutureEntity, COMPONENT_ID(T), initStrategy));
		return *this;
	}

	template<typename T>
	inline FutureEntityCommands& FutureEntityCommands::AddComponentWithData(const T& component)
	{
		m_CommandBuffer.AddCommand<AddComponentWithDataCommand<T>>(AddComponentWithDataCommand<T>(m_FutureEntity, component));
		return *this;
	}

	template<typename T>
	inline FutureEntityCommands& FutureEntityCommands::SetComponent(const T& component)
	{
		m_CommandBuffer.AddCommand<SetComponentCommand<T>>(SetComponentCommand<T>(m_FutureEntity, component));
		return *this;
	}

	template<typename T>
	inline FutureEntityCommands& FutureEntityCommands::RemoveComponent()
	{
		m_CommandBuffer.AddCommand<RemoveComponentCommand>(RemoveComponentCommand(m_FutureEntity, COMPONENT_ID(T)));
		return *this;
	}

	template<typename F>
	inline FutureEntityCommands& FutureEntityCommands::ExecuteFunction(F&& function)
	{
		m_CommandBuffer.AddCommand<FunctionExecutionCommand<F, true>>(FunctionExecutionCommand<F, true>(std::move(function), m_FutureEntity));
		return *this;
	}

	template<typename T>
	inline FutureEntityCommands& FutureEntityCommands::AddCommand(T&& command)
	{
		m_CommandBuffer.AddCommand<T>(std::move(command));
		return *this;
	}
}