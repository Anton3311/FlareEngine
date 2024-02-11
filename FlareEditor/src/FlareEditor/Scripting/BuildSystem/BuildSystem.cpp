#include "BuildSystem.h"

#include "Flare/Project/Project.h"
#include "FlarePlatform/Platform.h"

#include <filesystem>

namespace Flare
{
	std::filesystem::path s_MSBuildPath = L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe";

	static std::filesystem::path ModuleNameToProjectPath(const std::string& name)
	{
		return Project::GetActive()->Location / fmt::format("{0}/{0}.vcxproj", name);
	}

	static const wchar_t* GetConfigurationName()
	{
#ifdef FLARE_DEBUG
		return L"Debug";
#elif FLARE_RELEASE
		return L"Release";
#elif FLARE_DIST
		return L"Dist";
#endif
		return nullptr;
	}

	void Flare::BuildSystem::BuildModules()
	{
		std::wstring configuration = GetConfigurationName();

		Ref<Project> activeProject = Project::GetActive();
		for (const std::string& name : activeProject->ScriptingModules)
		{
			std::filesystem::path projectPath = ModuleNameToProjectPath(name);

			if (!std::filesystem::exists(projectPath))
			{
				FLARE_CORE_WARN("Project file '{0}' doesn't exist", projectPath.generic_string());
				continue;
			}

			ProcessCreationSettings settings;
			settings.Arguments = projectPath.wstring() + L"/target:Compile /p:configuration=" + configuration + L" /p:platform=x64";
			settings.WorkingDirectory = projectPath.parent_path();
			int32_t exitCode = Platform::CreateProcess(s_MSBuildPath, settings);

			if (exitCode != 0)
				FLARE_CORE_ERROR("MSBuild failed with exit code {0}", exitCode);
		}
	}

	void BuildSystem::LinkModules()
	{
		std::wstring configuration = GetConfigurationName();

		Ref<Project> activeProject = Project::GetActive();
		for (const std::string& name : activeProject->ScriptingModules)
		{
			std::filesystem::path projectPath = ModuleNameToProjectPath(name);

			if (!std::filesystem::exists(projectPath))
			{
				FLARE_CORE_WARN("Project file '{0}' doesn't exist", projectPath.generic_string());
				continue;
			}

			ProcessCreationSettings settings;
			settings.Arguments = projectPath.wstring() + L"/target:Link /p:configuration=" + configuration + L" /p:platform=x64";
			settings.WorkingDirectory = projectPath.parent_path();
			int32_t exitCode = Platform::CreateProcess(s_MSBuildPath, settings);

			if (exitCode != 0)
				FLARE_CORE_ERROR("MSBuild failed with exit code {0}", exitCode);
		}
	}
}
