#include "Window.h"

#include "FlarePlatform/Window.h"
#include "FlarePlatform/Windows/WindowsWindow.h"

namespace Flare
{
	Scope<Window> Window::Create(WindowProperties& properties)
	{
#ifdef FLARE_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(properties);
#endif
		return nullptr;
	}
}