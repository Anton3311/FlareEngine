#include "ScriptingEngine.h"

#include "Flare/Core/Log.h"

#include "FlareScripting/ScriptingModule.h"

#include <windows.h>

namespace Flare
{
	const std::string ScriptingEngine::s_ModuleLoaderFunctionName = "OnModuleLoaded";
	const std::string ScriptingEngine::s_ModuleUnloaderFunctionName = "OnModuleUnloaded";

	ScriptingEngine::Data ScriptingEngine::s_Data;

	void ScriptingEngine::Initialize()
	{
	}

	void ScriptingEngine::Shutdown()
	{
		UnloadAllModules();
	}

	void ScriptingEngine::ReloadModules()
	{
	}

	void ScriptingEngine::LoadModule(const std::filesystem::path& modulePath)
	{
		Ref<ScriptingModule> module = ScriptingModule::Create(modulePath);
		if (module->IsLoaded())
		{
			std::optional<ScriptingModuleFunction> onLoad = module->LoadFunction(s_ModuleLoaderFunctionName);
			std::optional<ScriptingModuleFunction> onUnload = module->LoadFunction(s_ModuleUnloaderFunctionName);

			ScriptingModuleData moduleData;
			moduleData.Config = ModuleConfiguration{};

			moduleData.Module = module;
			moduleData.OnLoad = onLoad.has_value() ? (ModuleEventFunction)onLoad.value() : std::optional<ModuleEventFunction>{};
			moduleData.OnUnload = onLoad.has_value() ? (ModuleEventFunction)onUnload.value() : std::optional<ModuleEventFunction>{};

			if (onLoad.has_value())
			{
				moduleData.OnLoad.value()(moduleData.Config);

				const std::vector<ScriptingType>& registeredTypes = *moduleData.Config.RegisteredTypes;

				for (const ScriptingType& type : registeredTypes)
				{
					FLARE_CORE_INFO("{0} {1}", type.Name, type.Size);
				}
			}

			s_Data.Modules.push_back(std::move(moduleData));
		}
	}

	void ScriptingEngine::UnloadAllModules()
	{
		for (auto& module : s_Data.Modules)
		{
			if (module.OnUnload.has_value())
				module.OnUnload.value()(module.Config);
		}

		s_Data.Modules.clear();
	}
}