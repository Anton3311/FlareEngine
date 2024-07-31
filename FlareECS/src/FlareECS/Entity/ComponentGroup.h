#pragma once

#include "FlareECS/Entity/Component.h"

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
}