#include "Flare/Core/Application.h"
#include "Flare/Core/CommandLineArguments.h"
#include "FlareCore/Log.h"

#include <stdint.h>

namespace Flare
{
	extern Scope<Application> CreateFlareApplication(CommandLineArguments arguments);
}

int main(int argc, const char* argv[])
{
	Flare::CommandLineArguments arguments;
	arguments.ArgumentsCount = argc;
	arguments.Arguments = argv;

	Flare::Log::Initialize();

	Flare::Scope<Flare::Application> application = CreateFlareApplication(arguments);
	application->Run();
	return 0;
}