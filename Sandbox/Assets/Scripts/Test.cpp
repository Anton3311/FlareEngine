#include "FlareScriptingCore/Flare.h"

#include "Flare/Core/Log.h"

#include <stdint.h>
#include <iostream>

namespace Sandbox
{
	struct HealthComponent
	{
		FLARE_COMPONENT(HealthComponent);
	};
	FLARE_COMPONENT_IMPL(HealthComponent);

	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
		}
	};

	FLARE_SYSTEM_IMPL(TestSystem);
}