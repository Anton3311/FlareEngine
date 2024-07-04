#include "Project.h"

#include "FlareCore/Assert.h"

#include "Flare/Core/Application.h"

#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Project/ProjectSerializer.h"
#include "Flare/Scripting/ScriptingEngine.h"

namespace Flare
{
	Ref<Project> s_Active;
	Signal<> Project::OnProjectOpen;
	Signal<> Project::OnUnloadActiveProject;
	bool Project::s_ProjectReloadScheduled = false;

	std::filesystem::path Project::s_ProjectFileExtension = ".flareproj";

	Ref<Project> Project::GetActive()
	{
		return s_Active;
	}

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

		FLARE_CORE_ASSERT(!s_ProjectReloadScheduled);

		s_ProjectReloadScheduled = true;
		Application::GetInstance().ExecuteAfterEndOfFrame([path]()
		{
			GraphicsContext::GetInstance().WaitForDevice();

			if (s_Active != nullptr)
				Project::OnUnloadActiveProject.Invoke();

			ScriptingEngine::UnloadAllModules();

			Ref<Project> project = CreateRef<Project>(path.parent_path());
			ProjectSerializer::Deserialize(project, path);

			s_Active = project;

			Project::OnProjectOpen.Invoke();

			s_ProjectReloadScheduled = false;
		});
	}

	const std::filesystem::path& Project::GetProjectFileExtension()
	{
		return s_ProjectFileExtension;
	}

	void Project::Save()
	{
		ProjectSerializer::Serialize(s_Active, s_Active->GetProjectFilePath());
	}

	std::filesystem::path Project::GetProjectFilePath()
	{
		return (Location / Name).replace_extension(s_ProjectFileExtension);
	}
}
