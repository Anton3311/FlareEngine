#pragma once

#include "Flare/Core/UUID.h"
#include "Flare/Scripting/ScriptingModule.h"

#include "FlareECS/World.h"

#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/SystemInfo.h"

#include <filesystem>

namespace Flare
{
	using ModuleEventFunction = void(*)(Internal::ModuleConfiguration&);

	struct ScriptingTypeInstance
	{
		size_t TypeIndex = SIZE_MAX;
		void* Instance = nullptr;
	};

	struct ScriptingModuleData
	{
		Internal::ModuleConfiguration Config;
		Ref<ScriptingModule> Module;

		std::optional<ModuleEventFunction> OnLoad;
		std::optional<ModuleEventFunction> OnUnload;

		std::vector<ScriptingTypeInstance> ScriptingInstances;
		std::unordered_map<std::string_view, size_t> TypeNameToIndex;
	};

	class ScriptingEngine
	{
	public:
		struct Data
		{
			World* CurrentWorld = nullptr;
			std::vector<ScriptingModuleData> Modules;
			std::vector<ComponentId> TemporaryQueryComponents;
		};
	public:
		static void Initialize();
		static void Shutdown();

		static void OnFrameStart(float deltaTime);

		static void SetCurrentECSWorld(World& world);

		static void ReloadModules();
		static void ReleaseScriptingInstances();

		static void LoadModule(const std::filesystem::path& modulePath);
		static void UnloadAllModules();

		static void RegisterComponents();
		static void RegisterSystems();

		inline static Data& GetData() { return s_Data; }
	private:
		static Data s_Data;

		static const std::string s_ModuleLoaderFunctionName;
		static const std::string s_ModuleUnloaderFunctionName;
	};
}