#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/QueryFilters.h"

#include <array>

namespace Flare
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
					m_Ids[index] = COMPONENT_ID(Components);
					index++;
				} (), ...);
		}
	public:
		constexpr const std::array<ComponentId, sizeof...(Components)>& GetIds() const { return m_Ids; }
	private:
		std::array<ComponentId, sizeof...(Components)> m_Ids;
	};

	template<typename... T>
	class FilteredComponentsGroup
	{
	public:
		using ComponentsArray = std::array<ComponentId, sizeof...(T)>;

		FilteredComponentsGroup()
		{
			size_t index = 0;

			([&]
			{
				T filter;
				m_Components[index++] = filter.Component;
			} (), ...);
		}
	public:
		constexpr const ComponentsArray& GetComponents() const { return m_Components; }
	private:
		ComponentsArray m_Components;
	};
}