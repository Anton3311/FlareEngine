#pragma once

#include "FlareECS/ECSContext.h"
#include "FlareECS/Entities.h"

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentGroup.h"

#include "FlareECS/Entity/Archetypes.h"

#include "FlareECS/Query/QueryFilters.h"
#include "FlareECS/Query/QueyrBuilder.h"
#include "FlareECS/Query/Query.h"

#include "FlareECS/System/SystemsManager.h"

#include <vector>
#include <string_view>

namespace Flare
{
	class FLAREECS_API World
	{
	public:
		World(ECSContext& context);
		~World();
		World(const World&) = delete;

		void MakeCurrent();

		template<typename... T>
		constexpr Entity CreateEntity(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
		{
			ComponentGroup<T...> group;
			return Entities.CreateEntity(ComponentSet(group.GetIds().data(), group.GetIds().size()), initStrategy);
		}

		template<typename... T>
		constexpr Entity CreateEntity(T ...components)
		{
			using ComponentPair = std::pair<ComponentId, void*>;
			std::array<ComponentPair, sizeof...(T)> componentPairs;

			size_t index = 0;
			([&]
			{
				componentPairs[index] = { COMPONENT_ID(T), &components };
				index++;
			} (), ...);

			std::sort(componentPairs.begin(), componentPairs.end(), [](const ComponentPair& a, const ComponentPair& b) -> bool
			{
				return a.first < b.first;
			});

			return Entities.CreateEntity(componentPairs.data(), sizeof...(T));
		}

		template<typename T>
		constexpr T& GetEntityComponent(Entity entity)
		{
			std::optional<void*> componentData = Entities.GetEntityComponent(entity, COMPONENT_ID(T));
			FLARE_CORE_ASSERT(componentData.has_value(), "Failed to get entity component");
			return *(T*)componentData.value();
		}

		template<typename T>
		constexpr const T& GetEntityComponent(Entity entity) const
		{
			std::optional<const void*> componentData = Entities.GetEntityComponent(entity, COMPONENT_ID(T));
			FLARE_CORE_ASSERT(componentData.has_value(), "Failed to get entity component");
			return *(const T*)componentData.value();
		}

		template<typename T>
		constexpr T* TryGetEntityComponent(Entity entity)
		{
			return (T*)Entities.GetEntityComponent(entity, COMPONENT_ID(T));
		}

		template<typename T>
		constexpr const T* TryGetEntityComponent(Entity entity) const
		{
			return (const T*) Entities.GetEntityComponent(entity, COMPONENT_ID(T));
		}

		template<typename T>
		constexpr bool AddEntityComponent(Entity entity, T data)
		{
			return Entities.AddEntityComponent(entity, COMPONENT_ID(T), &data);
		}

		template<typename T>
		constexpr bool RemoveEntityComponent(Entity entity)
		{
			return Entities.RemoveEntityComponent(entity, COMPONENT_ID(T));
		}

		template<typename T>
		constexpr bool HasComponent(Entity entity)
		{
			return Entities.HasComponent(entity, COMPONENT_ID(T));
		}

		void DeleteEntity(Entity entity);
		bool IsEntityAlive(Entity entity) const;
		const std::vector<ComponentId>& GetEntityComponents(Entity entity);
		Entity GetSingletonEntity(const Query& query);

		template<typename T>
		constexpr T& GetSingletonComponent()
		{
			auto result = Entities.GetSingletonComponent(COMPONENT_ID(T));
			FLARE_CORE_ASSERT(result.has_value(), "Failed to get singleton component");
			return *(T*)result.value();
		}

		template<typename T>
		constexpr T* TryGetSingletonComponent()
		{
			return (T*)Entities.GetSingletonComponent(COMPONENT_ID(T));
		}

		inline QueryTargetSelector NewQuery()
		{
			return QueryTargetSelector(m_Queries);
		}

		static World& GetCurrent();
	public:
		inline const Archetypes& GetArchetypes() const { return m_Archetypes; }
		inline const QueryCache& GetQueries() const { return m_Queries; }

		inline SystemsManager& GetSystemsManager() { return m_SystemsManager; }
		inline const SystemsManager& GetSystemsManager() const { return m_SystemsManager; }

		Flare::Entities Entities;
		Flare::Components& Components;
	private:
		Archetypes& m_Archetypes;
		QueryCache m_Queries;
		SystemsManager m_SystemsManager;
	};
}