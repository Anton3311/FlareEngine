#pragma once

#include "FlareECS/System/System.h"

namespace Flare
{
	class FLAREECS_API SystemsManager;
	struct FLAREECS_API SystemInitializer
	{
		using CreateSystemFunction = System*(*)();

		SystemInitializer(const char* name, CreateSystemFunction createSystem);
		~SystemInitializer();

		constexpr SystemId GetId() const { return m_Id; }

		static std::vector<SystemInitializer*>& GetInitializers();

		const char* TypeName;
		CreateSystemFunction CreateSystem;
	private:
		SystemId m_Id;
		friend class SystemsManager;
	};

#define FLARE_SYSTEM static Flare::SystemInitializer _SystemInitializer;
#define FLARE_IMPL_SYSTEM(systemName) Flare::SystemInitializer systemName::_SystemInitializer( \
	typeid(systemName).name(),                                                                 \
	[]() -> Flare::System* { return new systemName(); })
}