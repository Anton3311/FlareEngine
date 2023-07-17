#include "FlareScriptingCore/ScriptingType.h"

#include <stdint.h>

namespace Sandbox
{
	struct HealthComponent
	{
		FLARE_DEFINE_SCRIPTING_TYPE(HealthComponent);

		uint32_t Health;
		uint32_t MaxHealth;
	};
	FLARE_IMPL_SCRIPTING_TYPE(HealthComponent);
}