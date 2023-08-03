#include "Flare/Core/Log.h"

#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/ScriptingType.h"

#include "FlareScriptingCore/ECS/ComponentInfo.h"
#include "FlareScriptingCore/ECS/SystemInfo.h"
#include "FlareScriptingCore/ECS/EntityView.h"

FLARE_API void OnModuleLoaded(Flare::Scripting::ModuleConfiguration& config, Flare::Scripting::Bindings& bindings)
{
	Flare::Log::Initialize();

	config.RegisteredTypes = &Flare::Scripting::ScriptingType::GetRegisteredTypes();
	config.RegisteredSystems = &Flare::Scripting::SystemInfo::GetRegisteredSystems();
	config.RegisteredComponents = &Flare::Scripting::ComponentInfo::GetRegisteredComponents();

	config.TimeData = &Flare::Scripting::TimeData::Data;

	Flare::Scripting::Bindings::Instance = &bindings;
}

FLARE_API void OnModuleUnloaded(Flare::Scripting::ModuleConfiguration& config)
{
	Flare::Scripting::Bindings::Instance = nullptr;
}