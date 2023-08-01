#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/System.h"
#include "FlareECS/EntityId.h"

#include <array>
#include <optional>
#include <string_view>
#include <stdint.h>

namespace Flare::Internal
{
	struct WorldBindings
	{
		using CreateEntityFunction = Entity(*)(const ComponentId* components, size_t count);
		CreateEntityFunction CreateEntity;

		using GetEntityComponentFunction = void*(*)(Entity entity, ComponentId id);
		GetEntityComponentFunction GetEntityComponent;

		void* (*AddEntityComponent)(Entity entity, ComponentId component, const void* componentData, size_t componentDataSize);
		void(*RemoveEntityComponent)(Entity entity, ComponentId component);

		bool(*IsEntityAlive)(Entity entity);
		void(*DeleteEntity)(Entity entity);

		using FindSystemGroupFunction = std::optional<SystemGroupId>(*)(std::string_view name);
		FindSystemGroupFunction FindSystemGroup;

		static WorldBindings Bindings;
	};

	template<typename... Components>
	class ComponentGroup
	{
	public:
		ComponentGroup()
		{
			size_t index = 0;
			([&]
			{
				m_Ids[index] = Components::Info.Id;
				index++;
			} (), ...);
		}
	public:
		constexpr const std::array<ComponentId, sizeof...(Components)>& GetIds() const { return m_Ids; }
	private:
		std::array<ComponentId, sizeof...(Components)> m_Ids;
	};

	class World
	{
	public:
		static constexpr std::optional<SystemGroupId> FindSystemGroup(std::string_view name)
		{
			return WorldBindings::Bindings.FindSystemGroup(name);
		}

		template<typename... Components>
		static constexpr Entity CreateEntity()
		{
			ComponentGroup<Components...> group;
			return WorldBindings::Bindings.CreateEntity(group.GetIds().data(), group.GetIds().size());
		}

		template<typename ComponentT>
		inline static ComponentT& GetEntityComponent(Entity entity)
		{
			return *(ComponentT*)WorldBindings::Bindings.GetEntityComponent(entity, ComponentT::Info.Id);
		}

		static constexpr bool IsAlive(Entity entity)
		{
			return WorldBindings::Bindings.IsEntityAlive(entity);
		}
	};
}