#include "EditorApplication.h"

#include "Flare/Core/Log.h"

#include "Flare/Project/Project.h"
#include "Flare/Platform/Platform.h"

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
		else
		{
			std::optional<std::filesystem::path> projectPath = Platform::ShowOpenFileDialog(L"Flare Project (*.flareproj)\0*.flareproj\0");

			if (projectPath.has_value())
				Project::OpenProject(projectPath.value());
		}

		PushLayer(CreateRef<EditorLayer>());
	}

	EditorApplication::~EditorApplication()
	{
	}
}
