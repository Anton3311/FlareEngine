#include "ScriptingEngine.h"

#include "Flare/Core/Log.h"

#include "Flare/Scripting/ScriptingModule.h"
#include "Flare/Scripting/ScriptingBridge.h"
#include "Flare/Scene/Components.h"
#include "Flare/Project/Project.h"

#include "FlareScriptingCore/SystemInfo.h"
#include "FlareScriptingCore/ComponentInfo.h"

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
		for (const std::filesystem::path& modulePath : Project::GetActive()->ScriptingModules)
			ScriptingEngine::LoadModule(modulePath);

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
					FLARE_CORE_ASSERT(instance.TypeIndex < module.Config.RegisteredTypes->size(), "Instance has invalid type index");
					const Internal::ScriptingType* type = (*module.Config.RegisteredTypes)[instance.TypeIndex];
					type->Deleter(instance.Instance);
				}
			}

			module.ScriptingInstances.clear();
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
			moduleData.Config = Internal::ModuleConfiguration{};

			moduleData.Module = module;
			moduleData.OnLoad = onLoad.has_value() ? (ModuleEventFunction)onLoad.value() : std::optional<ModuleEventFunction>{};
			moduleData.OnUnload = onLoad.has_value() ? (ModuleEventFunction)onUnload.value() : std::optional<ModuleEventFunction>{};

			if (onLoad.has_value())
			{
				moduleData.OnLoad.value()(moduleData.Config);

				const std::vector<Internal::ScriptingType*>& registeredTypes = *moduleData.Config.RegisteredTypes;

				size_t typeIndex = 0;
				for (Internal::ScriptingType* type : registeredTypes)
				{
					moduleData.TypeNameToIndex.emplace(type->Name, typeIndex++);

					if (type->ConfigureSerialization)
						type->ConfigureSerialization(type->GetSerializationSettings());
				}

				ScriptingBridge::ConfigureModule(moduleData.Config);
			}

			s_Data.Modules.push_back(std::move(moduleData));
		}
	}

	void ScriptingEngine::UnloadAllModules()
	{
		s_Data.ShouldRegisterComponents = false;
		ReleaseScriptingInstances();

		for (auto& module : s_Data.Modules)
		{
			if (module.OnUnload.has_value())
				module.OnUnload.value()(module.Config);
		}

		s_Data.Modules.clear();
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
			for (Internal::ComponentInfo* component : *module.Config.RegisteredComponents)
			{
				auto typeIndexIterator = module.TypeNameToIndex.find(component->Name);
				FLARE_CORE_ASSERT(typeIndexIterator != module.TypeNameToIndex.end());

				const Internal::ScriptingType* type = (*module.Config.RegisteredTypes)[typeIndexIterator->second];

				if (component->AliasedName.has_value())
				{
					std::optional<ComponentId> aliasedComponent = s_Data.CurrentWorld->GetRegistry().FindComponnet(component->AliasedName.value());
					FLARE_CORE_ASSERT(aliasedComponent.has_value(), "Failed to find component");
					component->Id = aliasedComponent.value();
				}
				else
				{
					component->Id = s_Data.CurrentWorld->GetRegistry().RegisterComponent(component->Name, type->Size, type->Destructor);
					s_Data.ComponentIdToTypeIndex.emplace(component->Id, ScriptingEngine::Data::TypeIndex{ moduleIndex, typeIndexIterator->second });
				}
			}

			moduleIndex++;
		}

		s_Data.ShouldRegisterComponents = false;
	}

	void ScriptingEngine::RegisterSystems()
	{
		for (ScriptingModuleData& module : s_Data.Modules)
		{
			for (const Internal::SystemInfo* systemInfo : *module.Config.RegisteredSystems)
			{
				auto typeIndexIterator = module.TypeNameToIndex.find(systemInfo->Name);
				FLARE_CORE_ASSERT(typeIndexIterator != module.TypeNameToIndex.end());

				const Internal::ScriptingType* type = (*module.Config.RegisteredTypes)[typeIndexIterator->second];
				Internal::SystemBase* systemInstance = (Internal::SystemBase*)type->Constructor();

				module.ScriptingInstances.push_back(ScriptingTypeInstance{ typeIndexIterator->second, systemInstance });

				Internal::SystemConfiguration config(&s_Data.TemporaryQueryComponents);
				systemInstance->Configure(config);

				s_Data.CurrentWorld->RegisterSystem(
					s_Data.CurrentWorld->GetRegistry().CreateQuery(ComponentSet(
						s_Data.TemporaryQueryComponents.data(), 
						s_Data.TemporaryQueryComponents.size())),
					[systemInstance](EntityView view)
					{
						FLARE_CORE_ASSERT(s_Data.CurrentWorld != nullptr);
						ArchetypeRecord& record = s_Data.CurrentWorld->GetRegistry().GetArchetypeRecord(view.GetArchetype());

						size_t chunksCount = record.Storage.GetChunksCount();
						for (size_t i = 0; i < chunksCount; i++)
						{
							Internal::EntityView chunk(
								view.GetArchetype(),
								record.Storage.GetChunkBuffer(i),
								record.Storage.GetEntitySize(),
								record.Storage.GetEntitiesCountInChunk(i));

							systemInstance->Execute(chunk);
						}
					});

				s_Data.TemporaryQueryComponents.clear();
			}
		}
	}

	std::optional<const Internal::ScriptingType*> ScriptingEngine::FindComponentType(ComponentId id)
	{
		auto it = s_Data.ComponentIdToTypeIndex.find(id);
		if (it != s_Data.ComponentIdToTypeIndex.end())
			return (*s_Data.Modules[it->second.ModuleIndex].Config.RegisteredTypes)[it->second.TypeIndex];
		
		return {};
	}
}