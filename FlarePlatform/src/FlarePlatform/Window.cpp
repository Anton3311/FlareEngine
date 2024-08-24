#include "Window.h"

#include "FlarePlatform/Window.h"
#include "FlarePlatform/Windows/WindowsWindow.h"

namespace Flare
{
	Ref<Window> Window::Create(WindowProperties& properties)
	{
#ifdef FLARE_PLATFORM_WINDOWS
		return Ref<WindowsWindow>::New(properties);
#endif
		return nullptr;
	}
}