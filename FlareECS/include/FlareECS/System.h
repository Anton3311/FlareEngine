#pragma once

#include <stdint.h>

namespace Flare
{
	using SystemGroupId = uint32_t;
	using SystemId = uint32_t;

	struct ExecutionOrder
	{
		enum class Order : uint8_t
		{
			Before,
			After,
		};

		inline static ExecutionOrder After(uint32_t index)
		{
			return { Order::After, index };
		}

		inline static ExecutionOrder Before(uint32_t index)
		{
			return { Order::Before, index };
		}

		Order ExecutionOrder;
		uint32_t ItemIndex;
	};
}