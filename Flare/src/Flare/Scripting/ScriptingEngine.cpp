#include "ScriptingEngine.h"

#include "Flare/Core/Log.h"

#include "Flare/Scripting/ScriptingModule.h"
#include "Flare/Scripting/ScriptingBridge.h"

#include "FlareScriptingCore/SystemInfo.h"

#include "Flare/Scene/Components.h"

#include "FlareECS.h"

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

	void ScriptingEngine::SetCurrentECSWorld(World& world)
	{
		s_Data.CurrentWorld = &world;
	}

	void ScriptingEngine::ReloadModules()
	{
	}

	void ScriptingEngine::ReleaseScriptingInstances()
	{
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			for (ScriptingTypeInstance& instance : module.ScriptingInstances)
			{
				if (instance.Instance != nullptr)
				{
					FLARE_CORE_ASSERT(instance.TypeIndex < module.Config.RegisteredTypes->size(), "Instance has invalid type index");
					const ScriptingType* type = (*module.Config.RegisteredTypes)[instance.TypeIndex];
					type->Destructor(instance.Instance);
				}
			}
		}
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

				const std::vector<const ScriptingType*>& registeredTypes = *moduleData.Config.RegisteredTypes;

				size_t typeIndex = 0;
				for (const ScriptingType* type : registeredTypes)
					moduleData.TypeNameToIndex.emplace(type->Name, typeIndex++);

				ScriptingBridge::ConfigureModule(moduleData.Config);
			}

			s_Data.Modules.push_back(std::move(moduleData));
		}
	}

	void ScriptingEngine::UnloadAllModules()
	{
		ReleaseScriptingInstances();

		for (auto& module : s_Data.Modules)
		{
			if (module.OnUnload.has_value())
				module.OnUnload.value()(module.Config);
		}

		s_Data.Modules.clear();
	}

	void ScriptingEngine::RegisterSystems()
	{
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			for (const SystemInfo* systemInfo : *module.Config.RegisteredSystems)
			{
				auto typeIndexIterator = module.TypeNameToIndex.find(systemInfo->Name);

				FLARE_CORE_ASSERT(typeIndexIterator != module.TypeNameToIndex.end());

				const ScriptingType* type = (*module.Config.RegisteredTypes)[typeIndexIterator->second];
				SystemBase* systemInstance = (SystemBase*)type->Constructor();

				module.ScriptingInstances.push_back(ScriptingTypeInstance{ typeIndexIterator->second, systemInstance });

				SystemConfiguration config;
				systemInstance->Configure(config);

				s_Data.CurrentWorld->RegisterSystem(s_Data.CurrentWorld->CreateQuery<TransformComponent>(), [systemInstance](EntityView view)
				{
					systemInstance->Execute();
				});
			}
		}
	}
}