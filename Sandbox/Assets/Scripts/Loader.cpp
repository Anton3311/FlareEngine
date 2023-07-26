#include "Flare/Core/Log.h"

#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/SystemInfo.h"
#include "FlareScriptingCore/ComponentInfo.h"
#include "FlareScriptingCore/Bindings/ECS/EntityView.h"

FLARE_API void OnModuleLoaded(Flare::Internal::ModuleConfiguration& config)
{
	Flare::Log::Initialize();

	config.RegisteredTypes = &Flare::Internal::ScriptingType::GetRegisteredTypes();
	config.RegisteredSystems = &Flare::Internal::SystemInfo::GetRegisteredSystems();
	config.RegisteredComponents = &Flare::Internal::ComponentInfo::GetRegisteredComponents();

	config.WorldBindings = &Flare::Internal::WorldBindings::Bindings;
	config.EntityViewBindings = &Flare::Internal::EntityViewBindings::Bindings;
	config.TimeData = &Flare::Internal::TimeData::Data;

	config.InputBindings = &Flare::Internal::InputBindings::Bindings;
}

FLARE_API void OnModuleUnloaded(Flare::Internal::ModuleConfiguration& config)
{
}