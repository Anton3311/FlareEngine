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

		constexpr ComponentT& operator[](EntityViewElement& entity) const
		{
			return *(ComponentT*)(entity.GetEntityData() + m_ComponentOffset);
		}
	private:
		size_t m_ComponentOffset;
	};

	template<typename T>
	class OptionalComponentView
	{
	public:
		constexpr OptionalComponentView()
			: m_HasComponent(false), m_Offset(0) {}
		constexpr OptionalComponentView(size_t offset)
			: m_HasComponent(true), m_Offset(offset) {}

		constexpr std::optional<T*> operator[](EntityViewElement& entity) const
		{
			if (m_HasComponent)
				return (T*)(entity.GetEntityData() + m_Offset);
			return {};
		}

		constexpr T& GetOrDefault(EntityViewElement& entity, T& defaultValue) const
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