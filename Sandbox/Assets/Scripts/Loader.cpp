#include "Flare/Core/Log.h"

#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/SystemInfo.h"

FLARE_API void OnModuleLoaded(Flare::ModuleConfiguration& config)
{
	Flare::Log::Initialize();

	config.RegisteredTypes = &Flare::ScriptingType::GetRegisteredTypes();
	config.RegisteredSystems = &Flare::SystemInfo::GetRegisteredSystems();

	config.WorldBindings = &Flare::Bindings::WorldBindings::Bindings;
}

FLARE_API void OnModuleUnloaded(Flare::ModuleConfiguration& config)
{
}