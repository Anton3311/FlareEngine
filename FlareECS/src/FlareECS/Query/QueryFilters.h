#pragma once

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/ComponentInitializer.h"

#include <stdint.h>

namespace Flare
{
	enum class QueryFilterType : uint32_t
	{
		With = 0,
		Without = 1ui32 << 31ui32,
	};

	class QueryFilter
	{
	public:
		constexpr QueryFilter(ComponentId component)
			: Component(component) {}

		const ComponentId Component;
	};

	template<typename T>
	class With : public QueryFilter
	{
	public:
		constexpr With()
			: QueryFilter(COMPONENT_ID(T)) {}
	};

	template<typename T>
	class Without : public QueryFilter
	{
	public:
		constexpr Without()
			: QueryFilter(ComponentId(
				COMPONENT_ID(T).GetIndex() | (uint32_t)QueryFilterType::Without, 
				COMPONENT_ID(T).GetGeneration())) {}
	};
}