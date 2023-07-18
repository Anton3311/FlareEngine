#include "Project.h"

#include "Flare/Core/Assert.h"
#include "Flare/Project/ProjectSerializer.h"
#include "Flare/Scripting/ScriptingEngine.h"

namespace Flare
{
	Ref<Project> Project::s_Active;
	std::filesystem::path Project::s_ProjectFileExtension = ".flareproj";

	void Project::New(std::string_view name, const std::filesystem::path& path)
	{
		FLARE_CORE_ASSERT(std::filesystem::is_directory(path));

		std::filesystem::create_directories(path);

		Ref<Project> newProject = CreateRef<Project>(path);
		newProject->Name = name;
		newProject->StartScene = NULL_ASSET_HANDLE;

		ProjectSerializer::Serialize(newProject, newProject->GetProjectFilePath());
		s_Active = newProject;
	}

	void Project::OpenProject(const std::filesystem::path& path)
	{
		FLARE_CORE_ASSERT(!std::filesystem::is_directory(path));
		FLARE_CORE_ASSERT(path.extension() == s_ProjectFileExtension);

		ScriptingEngine::UnloadAllModules();

		Ref<Project> project = CreateRef<Project>(path.parent_path());
		ProjectSerializer::Deserialize(project, path);

		s_Active = project;

		ScriptingEngine::LoadModule(path.parent_path() / "bin/Debug-windows-x86_64/" / s_Active->Name / fmt::format("{0}.dll", s_Active->Name));
	}

	void Project::Save()
	{
		ProjectSerializer::Serialize(s_Active, s_Active->GetProjectFilePath());
	}
}
