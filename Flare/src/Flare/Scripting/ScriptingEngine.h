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
		Ref<ScriptingModule> Module;
		Internal::ModuleConfiguration Config;

		std::optional<ModuleEventFunction> OnLoad;
		std::optional<ModuleEventFunction> OnUnload;

		std::vector<ScriptingTypeInstance> ScriptingInstances;
		std::unordered_map<std::string_view, size_t> TypeNameToIndex;

		std::unordered_map<ComponentId, size_t> ComponentIdToTypeIndex;
	};

	class ScriptingEngine
	{
	public:
		struct Data
		{
			World* CurrentWorld = nullptr;
			size_t RegisteredComponentCount = 0;
			bool ShouldRegisterComponents = false;
			std::vector<ScriptingModuleData> Modules;
			std::vector<ComponentId> TemporaryQueryComponents;
		};
	public:
		static void Initialize();
		static void Shutdown();

		static void OnFrameStart(float deltaTime);

		static void SetCurrentECSWorld(World& world);

		static void LoadModules();
		static void ReleaseScriptingInstances();

		static void UnloadAllModules();

		static void RegisterComponents();
		static void RegisterSystems();

		static std::optional<const Internal::ScriptingType*> FindComponentType(ComponentId id);

		inline static Data& GetData() { return s_Data; }
	private:
		static void LoadModule(const std::filesystem::path& modulePath);
	private:
		static Data s_Data;

		static const std::string s_ModuleLoaderFunctionName;
		static const std::string s_ModuleUnloaderFunctionName;
	};
}