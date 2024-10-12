#pragma once

#include "FlareECS/Entities.h"

namespace Flare
{
	template<typename ComponentT>
	class ComponentView
	{
	public:
		ComponentView() = default;
		constexpr ComponentView(ComponentT* componentArray)
			: m_ComponentArray(componentArray) {}

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
}