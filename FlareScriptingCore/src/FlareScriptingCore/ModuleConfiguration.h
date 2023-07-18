#pragma once

#include "FlareScriptingCore/Defines.h"
#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/SystemInfo.h"

#include "FlareScriptingCore/Bindings/ECS/World.h"

#include <vector>

namespace Flare
{
	struct ModuleConfiguration
	{
		const std::vector<const ScriptingType*>* RegisteredTypes = nullptr;
		const std::vector<const SystemInfo*>* RegisteredSystems = nullptr;

		Bindings::WorldBindings* WorldBindings = nullptr;
	};
}