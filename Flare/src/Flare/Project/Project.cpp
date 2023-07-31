#include "Project.h"

#include "Flare/Core/Assert.h"
#include "Flare/Project/ProjectSerializer.h"
#include "Flare/Scripting/ScriptingEngine.h"

namespace Flare
{
	Ref<Project> Project::s_Active;
	Signal<> Project::OnProjectOpen;
	Signal<> Project::OnUnloadActiveProject;

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

		Project::OnUnloadActiveProject.Invoke();

		ScriptingEngine::UnloadAllModules();

		Ref<Project> project = CreateRef<Project>(path.parent_path());
		ProjectSerializer::Deserialize(project, path);

		s_Active = project;

		ScriptingEngine::LoadModules();

		Project::OnProjectOpen.Invoke();
	}

	void Project::Save()
	{
		ProjectSerializer::Serialize(s_Active, s_Active->GetProjectFilePath());
	}
}
