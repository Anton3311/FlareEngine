#include "Flare.h"

#include "Flare/Core/EntryPoint.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "SandboxLayer.h"

namespace Flare
{
	class SandboxApplication : public Application
	{
	public:
		SandboxApplication()
		{
			Renderer2D::Initialize();

			PushLayer(CreateRef<SandboxLayer>());
		}

		~SandboxApplication()
		{
			Renderer2D::Shutdown();
		}
	};
}

Flare::Scope<Flare::Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return Flare::CreateScope<Flare::SandboxApplication>();
}
