#include "Flare/Core/EntryPoint.h"

#include "FlareEditor/EditorApplication.h"

Flare::Scope<Flare::Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return Flare::CreateScope<Flare::EditorApplication>(arguments);
}
