#include "EditorApplication.h"

#include "Flare/Project/Project.h"
#include "FlareEditor/EditorLayer.h"

#include <filesystem>

namespace Flare
{
	EditorApplication::EditorApplication(CommandLineArguments arguments)
	{
		Application::GetInstance().GetWindow()->SetTitle("Flare Editor");

		if (arguments.ArgumentsCount >= 2)
		{
			std::filesystem::path projectPath = arguments[1];
			Project::OpenProject(projectPath);
		}

		PushLayer(CreateRef<EditorLayer>());
	}

	EditorApplication::~EditorApplication()
	{
	}
}
