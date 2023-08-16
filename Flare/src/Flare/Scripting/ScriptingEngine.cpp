#include "ScriptingEngine.h"

#include "Flare/Core/Log.h"

#include "Flare/Scripting/ScriptingModule.h"
#include "Flare/Scripting/ScriptingBridge.h"
#include "Flare/Scene/Components.h"
#include "Flare/Project/Project.h"

#include "FlareScriptingCore/ECS/SystemInfo.h"
#include "FlareScriptingCore/ECS/ComponentInfo.h"

#include "FlareECS/System/SystemInitializer.h"

#include "FlareECS.h"

namespace Flare
{
	const std::string s_ModuleLoaderFunctionName = "OnModuleLoaded";
	const std::string s_ModuleUnloaderFunctionName = "OnModuleUnloaded";

	ScriptingEngine::Data s_Data;

	void ScriptingEngine::Initialize()
	{
		ScriptingBridge::Initialize();
	}

	void ScriptingEngine::Shutdown()
	{
		UnloadAllModules();
	}

	void ScriptingEngine::OnFrameStart(float deltaTime)
	{
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			module.Config.TimeData->DeltaTime = deltaTime;
		}
	}

	void ScriptingEngine::SetCurrentECSWorld(World& world)
	{
		s_Data.CurrentWorld = &world;
		ScriptingBridge::SetCurrentWorld(world);
	}

	void ScriptingEngine::LoadModules()
	{
		FLARE_CORE_ASSERT(Project::GetActive());

		std::string_view configurationName = "";
		std::string_view platformName = "";
#ifdef FLARE_DEBUG
		configurationName = "Debug";
#elif defined(FLARE_RELEASE)
		configurationName = "Release";
#elif defined(FLARE_DIST)
		configurationName = "Dist";
#endif

#ifdef FLARE_PLATFORM_WINDOWS
		platformName = "windows";
#endif
		
		for (const std::string& moduleName : Project::GetActive()->ScriptingModules)
		{
			std::filesystem::path libraryPath = Project::GetActive()->Location
				/ fmt::format("bin/{0}-{1}-x86_64/{2}.dll", configurationName, platformName, moduleName);

			void* library = Platform::LoadSharedLibrary(libraryPath);
			FLARE_CORE_ASSERT(library);

			s_Data.LoadedSharedLibraries.push_back(library);
		}

		s_Data.ShouldRegisterComponents = true;
	}

	void ScriptingEngine::ReleaseScriptingInstances()
	{
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			for (ScriptingTypeInstance& instance : module.ScriptingInstances)
			{
				if (instance.Instance != nullptr)
				{
					const Scripting::ScriptingType* type = GetScriptingType(instance.Type);
					type->Deleter(instance.Instance);
				}
			}

			module.ScriptingInstances.clear();
		}
	}

	void ScriptingEngine::LoadModule(const std::filesystem::path& modulePath)
	{
		ScriptingModuleData moduleData;
		moduleData.Module.Load(modulePath);

		if (!moduleData.Module.IsLoaded())
			return;

		if (moduleData.Module.IsLoaded())
		{
			size_t moduleIndex = s_Data.Modules.size();

			moduleData.Config = Scripting::ModuleConfiguration{};

			moduleData.OnLoad = moduleData.Module.LoadFunction<OnModuleLoadFunction>(s_ModuleLoaderFunctionName);
			moduleData.OnUnload = moduleData.Module.LoadFunction<OnModuleUnloadFunction>(s_ModuleUnloaderFunctionName);

			if (moduleData.OnLoad.has_value())
			{
				moduleData.OnLoad.value()(moduleData.Config, ScriptingBridge::GetBindings());

				const std::vector<Scripting::ScriptingType*>& registeredTypes = *moduleData.Config.RegisteredTypes;

				size_t typeIndex = 0;
				for (Scripting::ScriptingType* type : registeredTypes)
				{
					s_Data.TypeNameToIndex.emplace(type->Name, ScriptingItemIndex(moduleIndex, typeIndex));

					if (type->ConfigureSerialization)
						type->ConfigureSerialization(type->GetSerializationSettings());

					typeIndex++;
				}
			}

			s_Data.Modules.push_back(std::move(moduleData));
		}
	}

	const Scripting::ScriptingType* ScriptingEngine::GetScriptingType(ScriptingItemIndex index)
	{
		return (*s_Data.Modules[index.ModuleIndex].Config.RegisteredTypes)[index.IndexInModule];
	}

	void ScriptingEngine::UnloadAllModules()
	{
		s_Data.ShouldRegisterComponents = false;
		ReleaseScriptingInstances();

		for (void* lib : s_Data.LoadedSharedLibraries)
			Platform::FreeSharedLibrary(lib);
		s_Data.LoadedSharedLibraries.clear();

		for (auto& module : s_Data.Modules)
		{
			if (module.OnUnload.has_value())
				module.OnUnload.value()(module.Config);
		}

		s_Data.ComponentIdToTypeIndex.clear();
		s_Data.Modules.clear();
		s_Data.TypeNameToIndex.clear();
		s_Data.SystemIndexToInstance.clear();
		s_Data.SystemNameToInstance.clear();
		s_Data.CurrentWorld = nullptr;
	}

	void ScriptingEngine::RegisterComponents()
	{
		if (s_Data.Modules.size() == 0)
			return;
		if (!s_Data.ShouldRegisterComponents)
			return;

		FLARE_CORE_ASSERT(s_Data.CurrentWorld != nullptr);

		size_t moduleIndex = 0;
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			FLARE_CORE_ASSERT(module.Module.IsLoaded());
			FLARE_CORE_ASSERT(module.Config.RegisteredComponents != nullptr);
			FLARE_CORE_ASSERT(module.Config.RegisteredTypes != nullptr);
			for (Scripting::ComponentInfo* component : *module.Config.RegisteredComponents)
			{
				auto typeIndexIterator = s_Data.TypeNameToIndex.find(std::string(component->Name));
				FLARE_CORE_ASSERT(typeIndexIterator != s_Data.TypeNameToIndex.end());
				FLARE_CORE_ASSERT(typeIndexIterator->second.ModuleIndex == moduleIndex);

				const Scripting::ScriptingType* type = (*module.Config.RegisteredTypes)[typeIndexIterator->second.IndexInModule];

				if (component->AliasedName.has_value())
				{
					std::optional<ComponentId> aliasedComponent = s_Data.CurrentWorld->GetRegistry().FindComponnet(component->AliasedName.value());
					FLARE_CORE_ASSERT(aliasedComponent.has_value(), "Failed to find component");
					component->Id = aliasedComponent.value();
				}
				else
				{
					component->Id = s_Data.CurrentWorld->GetRegistry().RegisterComponent(component->Name, type->Size, type->Destructor);
					s_Data.ComponentIdToTypeIndex.emplace(component->Id, typeIndexIterator->second);
				}
			}

			moduleIndex++;
		}

		s_Data.ShouldRegisterComponents = false;
	}

	void ScriptingEngine::CreateSystems()
	{
		size_t moduleIndex = 0;
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			module.FirstSystemInstance = module.ScriptingInstances.size();
			for (const Scripting::SystemInfo* systemInfo : *module.Config.RegisteredSystems)
			{
				auto typeIndexIterator = s_Data.TypeNameToIndex.find(std::string(systemInfo->Name));
				FLARE_CORE_ASSERT(typeIndexIterator != s_Data.TypeNameToIndex.end());

				const Scripting::ScriptingType* type = (*module.Config.RegisteredTypes)[typeIndexIterator->second.IndexInModule];
				Scripting::SystemBase* systemInstance = (Scripting::SystemBase*)type->Constructor();

				s_Data.SystemNameToInstance.emplace(systemInfo->Name, ScriptingItemIndex(moduleIndex, module.ScriptingInstances.size()));
				module.ScriptingInstances.push_back(ScriptingTypeInstance{ typeIndexIterator->second, systemInstance });
			}

			moduleIndex++;
		}
	}

	void ScriptingEngine::RegisterSystems()
	{
		std::string_view defaultGroupName = "Scripting Update";
		std::optional<SystemGroupId> defaultGroup = s_Data.CurrentWorld->GetSystemsManager().FindGroup(defaultGroupName);
		FLARE_CORE_ASSERT(defaultGroup.has_value());

		s_Data.CurrentWorld->GetSystemsManager().RegisterSystems(defaultGroup.value());

		SystemsManager& systems = s_Data.CurrentWorld->GetSystemsManager();
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			const auto& registeredSystems = *module.Config.RegisteredSystems;
			for (size_t systemIndex = 0; systemIndex < registeredSystems.size(); systemIndex++)
			{
				Scripting::SystemInfo* systemInfo = registeredSystems[systemIndex];
				ScriptingTypeInstance& instance = module.GetSystemInstance(systemIndex);
				Scripting::SystemBase& systemInstance = instance.As<Scripting::SystemBase>();

				const Scripting::ScriptingType* type = GetScriptingType(instance.Type);

				SystemId id = systems.RegisterSystem(type->Name,
					[system = &systemInstance](SystemExecutionContext& context)
					{
						system->OnUpdate();
					});

				systemInfo->Id = id;

				s_Data.SystemIndexToInstance.emplace(id, ScriptingItemIndex(
					instance.Type.ModuleIndex,
					module.FirstSystemInstance + systemIndex));
			}
		}

		std::vector<Scripting::SystemConfiguration> configs;
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			const auto& registeredSystems = *module.Config.RegisteredSystems;
			for (size_t systemIndex = 0; systemIndex < registeredSystems.size(); systemIndex++)
			{
				const Scripting::SystemInfo* systemInfo = registeredSystems[systemIndex];

				Scripting::SystemBase& systemInstance = module.GetSystemInstance(systemIndex).As<Scripting::SystemBase>();
				Scripting::SystemConfiguration& config = configs.emplace_back(defaultGroup.value());

				systemInstance.Configure(config);

				if (!systems.IsGroupIdValid(config.Group))
				{
					FLARE_CORE_ERROR("System '{0}' cannot be registered because the specified system group id '{1}' is not valid",
						systemInfo->Name, config.Group);
					continue;
				}

				systems.AddSystemToGroup(systemInfo->Id, config.Group);
			}
		}

		size_t configIndex = 0;
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			const auto& registeredSystems = *module.Config.RegisteredSystems;
			for (size_t systemIndex = 0; systemIndex < registeredSystems.size(); systemIndex++)
			{
				const Scripting::SystemInfo* systemInfo = registeredSystems[systemIndex];
				const Scripting::SystemConfiguration& config = configs[configIndex++];
				systems.AddSystemExecutionSettings(systemInfo->Id, &config.GetExecutionOrder());
			}
		}
	}

	std::optional<const Scripting::ScriptingType*> ScriptingEngine::FindType(std::string_view name)
	{
		auto it = s_Data.TypeNameToIndex.find(std::string(name));
		if (it == s_Data.TypeNameToIndex.end())
			return {};
		return GetScriptingType(it->second);
	}

	std::optional<const Scripting::ScriptingType*> ScriptingEngine::FindSystemType(uint32_t systemIndex)
	{
		auto it = s_Data.SystemIndexToInstance.find(systemIndex);
		if (it == s_Data.SystemIndexToInstance.end())
			return {};

		ScriptingItemIndex instanceIndex = it->second;
		return GetScriptingType(s_Data
			.Modules[instanceIndex.ModuleIndex]
			.ScriptingInstances[instanceIndex.IndexInModule].Type);
	}

	std::optional<Scripting::SystemBase*> ScriptingEngine::FindSystemByName(std::string_view name)
	{
		auto it = s_Data.SystemNameToInstance.find(name);
		if (it == s_Data.SystemNameToInstance.end())
			return {};

		ScriptingItemIndex index = it->second;
		void* instance = s_Data.Modules[index.ModuleIndex].ScriptingInstances[index.IndexInModule].Instance;
		return (Scripting::SystemBase*)instance;
	}

	std::optional<uint8_t*> ScriptingEngine::FindSystemInstance(uint32_t systemIndex)
	{
		auto it = s_Data.SystemIndexToInstance.find(systemIndex);
		if (it == s_Data.SystemIndexToInstance.end())
			return {};

		ScriptingItemIndex instanceIndex = s_Data.SystemIndexToInstance[systemIndex];
		return (uint8_t*) s_Data.Modules[instanceIndex.ModuleIndex].ScriptingInstances[instanceIndex.IndexInModule].Instance;
	}

	std::optional<const Scripting::ScriptingType*> ScriptingEngine::FindComponentType(ComponentId id)
	{
		auto it = s_Data.ComponentIdToTypeIndex.find(id);
		if (it != s_Data.ComponentIdToTypeIndex.end())
			return (*s_Data.Modules[it->second.ModuleIndex].Config.RegisteredTypes)[it->second.IndexInModule];
		
		return {};
	}

	ScriptingEngine::Data& ScriptingEngine::GetData()
	{
		return s_Data;
	}
}