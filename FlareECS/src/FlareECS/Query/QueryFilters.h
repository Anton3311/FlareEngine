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
}