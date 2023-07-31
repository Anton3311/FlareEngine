#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Core/Signal.h"

#include <filesystem>
#include <string>
#include <vector>
#include <string_view>

namespace Flare
{
	class Project
	{
	public:
		Project(const std::filesystem::path& location)
			: Location(location) {}
	public:
		static Ref<Project> GetActive() { return s_Active; }

		static void New(std::string_view name, const std::filesystem::path& path);
		static void OpenProject(const std::filesystem::path& path);
		static const std::filesystem::path& GetProjectFileExtension() { return s_ProjectFileExtension; }

		static void Save();

		inline std::filesystem::path GetProjectFilePath() { return (Location / Name).replace_extension(s_ProjectFileExtension); }
	public:
		std::string Name;
		const std::filesystem::path Location;
		AssetHandle StartScene;

		std::vector<std::string> ScriptingModules;
	public:
		static Ref<Project> s_Active;
		static Signal<> OnProjectOpen;
		static Signal<> OnUnloadActiveProject;
		static std::filesystem::path s_ProjectFileExtension;
	};
}