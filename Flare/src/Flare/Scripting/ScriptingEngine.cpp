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
		ScriptingModuleData moduleData;
		moduleData.Module.Load(modulePath);

		if (moduleData.Module.IsLoaded())
		{
			moduleData.Config = Internal::ModuleConfiguration{};

			moduleData.OnLoad = moduleData.Module.LoadFunction<ModuleEventFunction>(s_ModuleLoaderFunctionName);
			moduleData.OnUnload = moduleData.Module.LoadFunction<ModuleEventFunction>(s_ModuleUnloaderFunctionName);

			if (moduleData.OnLoad.has_value())
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
			FLARE_CORE_ASSERT(module.Module.IsLoaded());
			FLARE_CORE_ASSERT(module.Config.RegisteredComponents != nullptr);
			FLARE_CORE_ASSERT(module.Config.RegisteredTypes != nullptr);
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
		std::string_view defaultGroupName = "Scripting Update";
		std::optional<SystemGroupId> defaultGroup = s_Data.CurrentWorld->GetSystemsManager().FindGroup(defaultGroupName);
		FLARE_CORE_ASSERT(defaultGroup.has_value());

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

				Query query = s_Data.CurrentWorld->GetRegistry().CreateQuery(ComponentSet(
					s_Data.TemporaryQueryComponents.data(),
					s_Data.TemporaryQueryComponents.size()));

				SystemsManager& systems = s_Data.CurrentWorld->GetSystemsManager();
				systems.RegisterSystem((*module.Config.RegisteredTypes)[typeIndexIterator->second]->Name,
					defaultGroup.value(),
					query,
					nullptr,
					[systemInstance](SystemExecutionContext& context)
					{
						for (EntityView view : context.GetQuery())
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
						}
					},
					nullptr);

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