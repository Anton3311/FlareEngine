#pragma once

#include "Flare/Core/UUID.h"
#include "Flare/Scripting/ScriptingModule.h"

#include "FlareECS/World.h"

#include "FlareScriptingCore/ModuleConfiguration.h"
#include "FlareScriptingCore/Bindings/ECS/SystemInfo.h"

#include <filesystem>

namespace Flare
{
	using ModuleEventFunction = void(*)(Internal::ModuleConfiguration&);

	struct ScriptingItemIndex
	{
		size_t ModuleIndex;
		size_t IndexInModule;

		constexpr ScriptingItemIndex()
			: ModuleIndex(SIZE_MAX), IndexInModule(SIZE_MAX) {}
		constexpr ScriptingItemIndex(size_t moduleIndex, size_t index)
			: ModuleIndex(moduleIndex), IndexInModule(index) {}
	};

	struct ScriptingTypeInstance
	{
		ScriptingItemIndex Type = ScriptingItemIndex();
		void* Instance = nullptr;
	};

	struct ScriptingModuleData
	{
		ScriptingModule Module;
		Internal::ModuleConfiguration Config;

		std::optional<ModuleEventFunction> OnLoad;
		std::optional<ModuleEventFunction> OnUnload;

		std::vector<ScriptingTypeInstance> ScriptingInstances;
	};

	class ScriptingEngine
	{
	public:

		struct Data
		{
			World* CurrentWorld = nullptr;
			bool ShouldRegisterComponents = false;
			std::vector<ScriptingModuleData> Modules;
			std::vector<ComponentId> TemporaryQueryComponents;

			std::unordered_map<ComponentId, ScriptingItemIndex> ComponentIdToTypeIndex;
			std::unordered_map<uint32_t, ScriptingItemIndex> SystemIndexToInstance;
			std::unordered_map<std::string, ScriptingItemIndex> TypeNameToIndex;
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
		
		static std::optional<const Internal::ScriptingType*> FindType(std::string_view name);
		static std::optional<const Internal::ScriptingType*> FindSystemType(uint32_t systemIndex);
		static std::optional<uint8_t*> FindSystemInstance(uint32_t systemIndex);

		static std::optional<const Internal::ScriptingType*> FindComponentType(ComponentId id);

		inline static Data& GetData() { return s_Data; }
	private:
		static void LoadModule(const std::filesystem::path& modulePath);
		static const Internal::ScriptingType* GetScriptingType(ScriptingItemIndex index);
	private:
		static Data s_Data;

		static const std::string s_ModuleLoaderFunctionName;
		static const std::string s_ModuleUnloaderFunctionName;
	};
}