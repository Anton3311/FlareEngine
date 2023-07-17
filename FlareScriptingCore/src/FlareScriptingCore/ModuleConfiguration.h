#pragma once

#include "FlareScriptingCore/Defines.h"
#include "FlareScriptingCore/ScriptingType.h"

#include <vector>

namespace Flare
{
	struct ModuleConfiguration
	{
		const std::vector<ScriptingType>* RegisteredTypes;
	};
}