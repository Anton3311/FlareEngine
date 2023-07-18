#pragma once

#include <stdint.h>

namespace Flare::Bindings
{
	using ComponentId = size_t;
	constexpr ComponentId INVALID_COMPONENT_ID = SIZE_MAX;
}

#ifndef FLARE_SCRIPTING_CORE_NO_MACROS
	#define FLARE_COMPONENT static Flare::ComponentId Id;
	#define FLARE_COMPONENT_IMPL(component) Flare::ComponentId components::Id = Flare::INVALID_COMPONENT_ID;
#endif
