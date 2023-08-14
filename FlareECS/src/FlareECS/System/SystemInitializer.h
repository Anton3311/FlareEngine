#pragma once

#include "FlareECS/System/System.h"

namespace Flare
{
	struct FLAREECS_API SystemInitializer
	{
		using CreateSystemFunction = System*(*)();

		SystemInitializer(const char* name, CreateSystemFunction createSystem);
		~SystemInitializer();

		static std::vector<SystemInitializer*>& GetInitializers();

		const char* TypeName;
		CreateSystemFunction CreateSystem;
	};

#define FLARE_SYSTEM static Flare::SystemInitializer _SystemInitializer;
#define FLARE_IMPL_SYSTEM(systemName) Flare::SystemInitializer systemName::_SystemInitializer( \
	typeid(systemName).name(),                                                                 \
	[]() -> Flare::System* { return new systemName(); })
}