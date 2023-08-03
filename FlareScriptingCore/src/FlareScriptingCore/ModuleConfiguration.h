#pragma once

#include "FlareScriptingCore/Defines.h"
#include "FlareScriptingCore/ScriptingType.h"

#include "FlareScriptingCore/Bindings/ECS/ECS.h"
#include "FlareScriptingCore/Bindings/ECS/EntityView.h"
#include "FlareScriptingCore/Bindings/ECS/World.h"
#include "FlareScriptingCore/Bindings/ECS/ComponentInfo.h"
#include "FlareScriptingCore/Bindings/ECS/SystemInfo.h"

#include "FlareScriptingCore/Bindings/Time.h"
#include "FlareScriptingCore/Bindings/Input.h"

#include <vector>

namespace Flare::Internal
{
	struct ModuleConfiguration
	{
		const std::vector<ScriptingType*>* RegisteredTypes = nullptr;
		const std::vector<SystemInfo*>* RegisteredSystems = nullptr;
		const std::vector<ComponentInfo*>* RegisteredComponents = nullptr;

		Internal::WorldBindings* WorldBindings = nullptr;
		Internal::EntityViewBindings* EntityViewBindings = nullptr;
		Internal::TimeData* TimeData = nullptr;
		Internal::InputBindings* InputBindings = nullptr;
	};
}