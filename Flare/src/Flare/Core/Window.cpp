#include "Window.h"

#include <Flare/Platform/Windows/WindowsWindow.h>

namespace Flare
{
	Scope<Window> Window::Create(WindowProperties& properties)
	{
#ifdef FL_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(properties);
#endif
	}
}