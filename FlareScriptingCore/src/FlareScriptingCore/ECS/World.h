#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/System.h"
#include "FlareECS/EntityId.h"

#include "FlareECS/ComponentGroup.h"

#include "FlareScriptingCore/ECS/Query.h"
#include "FlareScriptingCore/ECS/EntityView.h"
#include "FlareScriptingCore/Bindings.h"

#include <array>
#include <optional>
#include <string_view>
#include <functional>

#include <stdint.h>

namespace Flare::Scripting
{
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

		constexpr static Entity GetSingletonEntity(Query query)
		{
			return Bindings::Instance->GetSingletonEntity(query.GetId());
		}

		template<typename T>
		constexpr static T* GetSingletonComponent()
		{
			return (T*)Bindings::Instance->GetSingletonComponent(T::Info.Id);
		}

		static constexpr bool IsAlive(Entity entity)
		{
			return Bindings::Instance->IsEntityAlive(entity);
		}

		static constexpr void ForEachChunk(Query query, const std::function<void(EntityView&)>& perChunk)
		{
			Bindings::Instance->ForEachChunkInQuery(query.GetId(), perChunk);
		}
	};
}