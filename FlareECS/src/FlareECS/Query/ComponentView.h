#pragma once

#include "FlareECS/Entities.h"

#include "FlareECS/Query/QueryChunkEntity.h"

namespace Flare
{
	template<typename ComponentT>
	class ComponentView
	{
	public:
		ComponentView() = default;
		constexpr ComponentView(ComponentT* componentArray)
			: m_ComponentArray(componentArray) {}

		constexpr ComponentT& operator[](const QueryChunkEntity& chunkEntity) const
		{
			return m_ComponentArray[chunkEntity.GetEntityIndex()];
		}

		constexpr ComponentT& operator[](size_t index) const
		{
			return m_ComponentArray[index];
		}
	private:
		ComponentT* m_ComponentArray = nullptr;
	};

	template<typename T>
	struct ComponentViewUnderlyingType
	{
		using Type = void;
	};

	template<typename T>
	struct ComponentViewUnderlyingType<ComponentView<T>>
	{
		using Type = T;
	};

	template<typename T>
	constexpr bool IsComponentView = false;

	template<typename T>
	constexpr bool IsComponentView<ComponentView<T>> = true;

	template<typename T>
	class OptionalComponentView
	{
	public:
		constexpr OptionalComponentView()
			: m_HasComponent(false), m_Offset(0) {}
		constexpr OptionalComponentView(size_t offset)
			: m_HasComponent(true), m_Offset(offset) {}

		constexpr std::optional<T*> operator[](QueryChunkEntity & entity) const
		{
			if (m_HasComponent)
				return (T*)(entity.GetEntityData() + m_Offset);
			return {};
		}

		constexpr bool HasComponent() const { return m_HasComponent; }

		constexpr T& GetOrDefault(QueryChunkEntity & entity, T& defaultValue) const
		{
			if (m_HasComponent)
				return *(T*)(entity.GetEntityData() + m_Offset);
			return defaultValue;
		}
	private:
		bool m_HasComponent;
		size_t m_Offset;
	};
}