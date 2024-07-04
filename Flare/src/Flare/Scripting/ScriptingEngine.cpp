#include "ScriptingEngine.h"

#include "FlareCore/Log.h"

#include "Flare/Project/Project.h"

#include "FlarePlatform/Platform.h"

#include <vector>

namespace Flare
{
	struct ScriptingEngineData
	{
		std::vector<void*> LoadedSharedLibraries;
	};

	ScriptingEngineData s_ScriptingData;

	void ScriptingEngine::Initialize()
	{
	}

	void ScriptingEngine::Shutdown()
	{
		UnloadAllModules();
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
}