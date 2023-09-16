#include "ScriptingEngine.h"

#include "FlareCore/Log.h"

#include "Flare/Scene/Components.h"
#include "Flare/Project/Project.h"

#include "FlarePlatform/Platform.h"

#include "FlareECS/System/SystemInitializer.h"

#include "FlareECS.h"

namespace Flare
{
	const std::string s_ModuleLoaderFunctionName = "OnModuleLoaded";
	const std::string s_ModuleUnloaderFunctionName = "OnModuleUnloaded";

	ScriptingEngine::Data s_ScriptingData;

	void ScriptingEngine::Initialize()
	{
	}

	void ScriptingEngine::Shutdown()
	{
		UnloadAllModules();
	}

	void ScriptingEngine::SetCurrentECSWorld(World& world)
	{
		s_ScriptingData.CurrentWorld = &world;
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
				/ fmt::format("bin/{0}-{1}-x86_64/{2}.dll",
					configurationName,
					platformName,
					moduleName);

			void* library = Platform::LoadSharedLibrary(libraryPath);
			if (!library)
			{
				FLARE_CORE_ERROR("Failed to load module '{0}'", moduleName);
				continue;
			}

			s_ScriptingData.LoadedSharedLibraries.push_back(library);
		}
	}

	void ScriptingEngine::UnloadAllModules()
	{
		for (void* lib : s_ScriptingData.LoadedSharedLibraries)
			Platform::FreeSharedLibrary(lib);
		s_ScriptingData.LoadedSharedLibraries.clear();
	}

	void ScriptingEngine::RegisterSystems()
	{
		std::string_view defaultGroupName = "Scripting Update";
		std::optional<SystemGroupId> defaultGroup = s_ScriptingData.CurrentWorld->GetSystemsManager().FindGroup(defaultGroupName);
		FLARE_CORE_ASSERT(defaultGroup.has_value());

		s_ScriptingData.CurrentWorld->GetSystemsManager().RegisterSystems(defaultGroup.value());
	}

	ScriptingEngine::Data& ScriptingEngine::GetData()
	{
		return s_ScriptingData;
	}
}