#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Core/Signal.h"

#include <filesystem>
#include <string>
#include <vector>
#include <string_view>

namespace Flare
{
	class FLARE_API Project
	{
	public:
		Project(const std::filesystem::path& location)
			: Location(location) {}
	public:
		static Ref<Project> GetActive();

		static void New(std::string_view name, const std::filesystem::path& path);
		static void OpenProject(const std::filesystem::path& path);
		static const std::filesystem::path& GetProjectFileExtension();

		static void Save();

		std::filesystem::path GetProjectFilePath();
	public:
		std::string Name;
		const std::filesystem::path Location;
		AssetHandle StartScene;

		std::vector<std::string> ScriptingModules;
		std::vector<std::filesystem::path> Modules;
	public:
		static Signal<> OnProjectOpen;
		static Signal<> OnUnloadActiveProject;
		static std::filesystem::path s_ProjectFileExtension;
	};
}