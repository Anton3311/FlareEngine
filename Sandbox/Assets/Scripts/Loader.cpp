#include "Flare/Core/Log.h"

#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/ScriptingType.h"

#include "FlareScriptingCore/ECS/ComponentInfo.h"
#include "FlareScriptingCore/ECS/SystemInfo.h"
#include "FlareScriptingCore/ECS/EntityView.h"

FLARE_API void OnModuleLoaded(Flare::Internal::ModuleConfiguration& config, Flare::Internal::Bindings& bindings)
{
	Flare::Log::Initialize();

	config.RegisteredTypes = &Flare::Internal::ScriptingType::GetRegisteredTypes();
	config.RegisteredSystems = &Flare::Internal::SystemInfo::GetRegisteredSystems();
	config.RegisteredComponents = &Flare::Internal::ComponentInfo::GetRegisteredComponents();

	config.TimeData = &Flare::Internal::TimeData::Data;

	Flare::Internal::Bindings::Instance = &bindings;
}

FLARE_API void OnModuleUnloaded(Flare::Internal::ModuleConfiguration& config)
{
	Flare::Internal::Bindings::Instance = nullptr;
}