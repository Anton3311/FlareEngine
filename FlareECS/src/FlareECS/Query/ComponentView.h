#pragma once

#include "FlareECS/Entities.h"
#include "FlareECS/Query/EntityViewIterator.h"

namespace Flare
{
	template<typename ComponentT>
	class ComponentView
	{
	public:
		constexpr ComponentView(size_t offset)
			: m_ComponentOffset(offset) {}

		constexpr ComponentT& operator[](EntityViewElement& entity)
		{
			return *(ComponentT*)(entity.GetEntityData() + m_ComponentOffset);
		}
	private:
		size_t m_ComponentOffset;
	};
}