#include <Flare/Core/Application.h>

using namespace Flare;

class SandboxApplication : public Application
{
public:
	SandboxApplication()
	{
		
	}

public:
	virtual void OnUpdate() override
	{

	}
};

int main()
{
	SandboxApplication application;
	application.Run();
	return 0;
}