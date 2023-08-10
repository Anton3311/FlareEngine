#pragma once

#include "FlareECS/Registry.h"
#include "FlareECS/Entity/Component.h"

#include "FlareECS/ComponentGroup.h"

#include "FlareECS/QueryFilters.h"
#include "FlareECS/Query/Query.h"

#include "FlareECS/System/SystemsManager.h"

#include <vector>
#include <string_view>

#define FLARE_COMPONENT static Flare::RegisteredComponentInfo Info;
#define FLARE_COMPONENT_IMPL(name) Flare::RegisteredComponentInfo name::Info;

namespace Flare
{
	class FLAREECS_API World
	{
	public:
		World();
		~World();
		World(const World&) = delete;

		template<typename ComponentT>
		constexpr void RegisterComponent()
		{
			ComponentT::Info.Id = m_Registry.RegisterComponent(typeid(ComponentT).name(), 
				sizeof(ComponentT), [](void* component) { ((ComponentT*)component)->~ComponentT(); });
		}

		template<typename... T>
		constexpr Entity CreateEntity()
		{
			ComponentId ids[sizeof...(T)];

			size_t index = 0;
			([&]
			{
				ids[index++] = T::Info.Id;
			} (), ...);

			return m_Registry.CreateEntity(ComponentSet(ids, sizeof...(T)));
		}

		template<typename T>
		constexpr T& GetEntityComponent(Entity entity)
		{
			std::optional<void*> componentData = m_Registry.GetEntityComponent(entity, T::Info.Id);
			FLARE_CORE_ASSERT(componentData.has_value(), "Failed to get entity component");
			return *(T*)componentData.value();
		}

		template<typename T>
		constexpr std::optional<T*> TryGetEntityComponent(Entity entity)
		{
			std::optional<void*> componentData = m_Registry.GetEntityComponent(entity, T::Info.Id);
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
			return m_Registry.AddEntityComponent(entity, T::Info.Id, &data);
		}

		template<typename T>
		constexpr bool RemoveEntityComponent(Entity entity)
		{
			return m_Registry.RemoveEntityComponent(entity, T::Info.Id);
		}
		
		template<typename T>
		constexpr bool HasComponent(Entity entity)
		{
			return m_Registry.HasComponent(entity, T::Info.Id);
		}

		inline void DeleteEntity(Entity entity)
		{
			m_Registry.DeleteEntity(entity);
		}

		inline bool IsEntityAlive(Entity entity) const
		{
			return m_Registry.IsEntityAlive(entity);
		}

		inline const std::vector<ComponentId>& GetEntityComponents(Entity entity)
		{
			return m_Registry.GetEntityComponents(entity);
		}

		constexpr Entity GetSingletonEntity(const Query& query)
		{
			return m_Registry.GetSingletonEntity(query).value_or(Entity());
		}

		template<typename T>
		constexpr T& GetSingletonComponent()
		{
			auto result = m_Registry.GetSingleComponent(T::Info.Id);
			FLARE_CORE_ASSERT(result.has_value(), "Failed to get singleton component");
			return *(T*)result.value();
		}

		template<typename T>
		constexpr std::optional<T*> TryGetSingletonComponent()
		{
			auto result = m_Registry.GetSingleComponent(T::Info.Id);
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