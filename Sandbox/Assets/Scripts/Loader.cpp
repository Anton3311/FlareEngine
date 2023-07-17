#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/ModuleEntryPoint.h"
#include "FlareScriptingCore/ScriptingType.h"

FLARE_API void OnModuleLoaded(Flare::ModuleConfiguration& config)
{
	config.RegisteredTypes = &Flare::ScriptingType::GetRegisteredTypes();
}

FLARE_API void OnModuleUnloaded(Flare::ModuleConfiguration& config)
{

}