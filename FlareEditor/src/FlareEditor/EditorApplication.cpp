#include "EditorApplication.h"

#include "FlareCore/Log.h"
#include "Flare/Project/Project.h"

#include "FlarePlatform/Platform.h"

#include "FlareEditor/EditorLayer.h"

#include <filesystem>

namespace Flare
{
	EditorApplication::EditorApplication(CommandLineArguments arguments)
		: Application(arguments)
	{
		Application::GetInstance().GetWindow()->SetTitle("Flare Editor");
		PushLayer(CreateRef<EditorLayer>());
	}

	EditorApplication::~EditorApplication()
	{
	}
}
