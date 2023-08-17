#pragma once

#include "FlareECS/Registry.h"
#include "FlareECS/Entity/Component.h"

#include "FlareECS/Entity/ComponentGroup.h"

#include "FlareECS/Query/QueryFilters.h"
#include "FlareECS/Query/Query.h"

#include "FlareECS/System/SystemsManager.h"

#include <vector>
#include <string_view>

namespace Flare
{
	class FLAREECS_API World
	{
	public:
		World();
		~World();
		World(const World&) = delete;

		template<typename... T>
		constexpr Entity CreateEntity(ComponentInitializationStrategy initStrategy = ComponentInitializationStrategy::DefaultConstructor)
		{
			ComponentId ids[sizeof...(T)];

			size_t index = 0;
			([&]
			{
				ids[index++] = COMPONENT_ID(T);
			} (), ...);

			return m_Registry.CreateEntity(ComponentSet(ids, sizeof...(T)), initStrategy);
		}

		template<typename T>
		constexpr T& GetEntityComponent(Entity entity)
		{
			std::optional<void*> componentData = m_Registry.GetEntityComponent(entity, COMPONENT_ID(T));
			FLARE_CORE_ASSERT(componentData.has_value(), "Failed to get entity component");
			return *(T*)componentData.value();
		}

		template<typename T>
		constexpr std::optional<T*> TryGetEntityComponent(Entity entity)
		{
			std::optional<void*> componentData = m_Registry.GetEntityComponent(entity, COMPONENT_ID(T));
			if (componentData.has_value())
				return (T*)componentData.value();
			return {};
		}

		template<typename T>
		constexpr std::optional<T*> TryGetEntityComponent(Entity entity, ComponentId component)
		{
			if (T::Id != component)
				return {};

			return TryGetEntityComponent<T>(entity);
		}

		template<typename T>
		constexpr bool AddEntityComponent(Entity entity, const T& data)
		{
			return m_Registry.AddEntityComponent(entity, COMPONENT_ID(T), &data);
		}

		template<typename T>
		constexpr bool RemoveEntityComponent(Entity entity)
		{
			return m_Registry.RemoveEntityComponent(entity, COMPONENT_ID(T));
		}
		
		template<typename T>
		constexpr bool HasComponent(Entity entity)
		{
			return m_Registry.HasComponent(entity, COMPONENT_ID(T));
		}

		void DeleteEntity(Entity entity);
		bool IsEntityAlive(Entity entity) const;
		const std::vector<ComponentId>& GetEntityComponents(Entity entity);
		Entity GetSingletonEntity(const Query& query);

		template<typename T>
		constexpr T& GetSingletonComponent()
		{
			auto result = m_Registry.GetSingleComponent(COMPONENT_ID(T));
			FLARE_CORE_ASSERT(result.has_value(), "Failed to get singleton component");
			return *(T*)result.value();
		}

		template<typename T>
		constexpr std::optional<T*> TryGetSingletonComponent()
		{
			auto result = m_Registry.GetSingleComponent(COMPONENT_ID(T));
			if (result.has_value())
				return *(T*)result.value();
			return {};
		}

		template<typename... T>
		constexpr Query CreateQuery()
		{
			FilteredComponentsGroup<T...> components;
			return m_Registry.CreateQuery(ComponentSet(components.GetComponents().data(), components.GetComponents().size()));
		}

		static World& GetCurrent();
	public:
		Registry& GetRegistry();

		SystemsManager& GetSystemsManager();
		const SystemsManager& GetSystemsManager() const;
	private:
		Registry m_Registry;
		SystemsManager m_SystemsManager;
	};
}