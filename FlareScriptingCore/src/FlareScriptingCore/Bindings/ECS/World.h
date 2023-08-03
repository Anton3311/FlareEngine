#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/System.h"
#include "FlareECS/EntityId.h"

#include "FlareScriptingCore/Bindings/ECS/EntityView.h"
#include "FlareScriptingCore/Bindings.h"

#include <array>
#include <optional>
#include <string_view>
#include <functional>

#include <stdint.h>

namespace Flare::Internal
{
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
			return Bindings::Instance->FindSystemGroup(name);
		}

		template<typename... Components>
		static constexpr Entity CreateEntity()
		{
			ComponentGroup<Components...> group;
			return Bindings::Instance->CreateEntity(group.GetIds().data(), group.GetIds().size());
		}

		template<typename ComponentT>
		inline static ComponentT& GetEntityComponent(Entity entity)
		{
			return *(ComponentT*)Bindings::Instance->GetEntityComponent(entity, ComponentT::Info.Id);
		}

		static constexpr bool IsAlive(Entity entity)
		{
			return Bindings::Instance->IsEntityAlive(entity);
		}
	};
}