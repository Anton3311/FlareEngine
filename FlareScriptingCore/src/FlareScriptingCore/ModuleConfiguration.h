#pragma once

#include "FlareScriptingCore/Defines.h"
#include "FlareScriptingCore/ScriptingType.h"

#include "FlareScriptingCore/ECS/EntityView.h"
#include "FlareScriptingCore/ECS/World.h"
#include "FlareScriptingCore/ECS/ComponentInfo.h"
#include "FlareScriptingCore/ECS/SystemInfo.h"

#include "FlareScriptingCore/Time.h"
#include "FlareScriptingCore/Input.h"

#include <vector>

namespace Flare::Scripting
{
	struct ModuleConfiguration
	{
		const std::vector<ScriptingType*>* RegisteredTypes = nullptr;
		const std::vector<SystemInfo*>* RegisteredSystems = nullptr;
		const std::vector<ComponentInfo*>* RegisteredComponents = nullptr;

		Scripting::TimeData* TimeData = nullptr;
	};
}