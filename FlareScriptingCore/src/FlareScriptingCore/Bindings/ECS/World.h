#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/System.h"

#include "FlareScriptingCore/Bindings/ECS/ECS.h"

#include <array>
#include <optional>
#include <string_view>
#include <stdint.h>

namespace Flare::Internal
{
	struct Entity
	{
	public:
		constexpr Entity()
			: m_Index(UINT32_MAX), m_Generation(UINT16_MAX), m_Dummy(UINT16_MAX) {}

		constexpr uint32_t GetIndex() const { return m_Index; }
		constexpr uint32_t GetGeneration() const { return m_Generation; }
	private:
		uint32_t m_Index;
		uint16_t m_Generation;
		uint16_t m_Dummy;
	};

	struct WorldBindings
	{
		using CreateEntityFunction = Entity(*)(ComponentId* components, size_t count);
		CreateEntityFunction CreateEntity;

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
			WorldBindings::Bindings.CreateEntity(group.GetIds().data(), group.GetIds().size());
		}

		static constexpr bool IsAlive(Entity entity)
		{
			return WorldBindings::Bindings.IsEntityAlive(entity);
		}
	};
}