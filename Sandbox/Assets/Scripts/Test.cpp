#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/SystemInfo.h"

#include "Flare/Core/Log.h"

#include <stdint.h>
#include <iostream>

namespace Sandbox
{
	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
		}

		virtual void Execute() override
		{
		}
	};

	FLARE_SYSTEM_IMPL(TestSystem);
}