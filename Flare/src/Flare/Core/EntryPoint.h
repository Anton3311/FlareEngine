#include <Flare/Core/Application.h>

#include <stdint.h>

namespace Flare
{
	struct CommandLineArguments
	{
		const char** Arguments = nullptr;
		uint32_t ArgumentsCount = 0;

		const char* operator[](uint32_t index)
		{
			return Arguments[index];
		}
	};

	extern Scope<Application> CreateFlareApplication(CommandLineArguments arguments);
}

int main(int argc, const char* argv[])
{
	Flare::CommandLineArguments arguments;
	arguments.ArgumentsCount = argc;
	arguments.Arguments = argv;

	Flare::Scope<Flare::Application> application = CreateFlareApplication(arguments);
	application->Run();
	return 0;
}