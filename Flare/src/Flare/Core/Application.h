#pragma once

#include "Flare/Core/Window.h"

namespace Flare
{
	class Application
	{
	public:
		Application();

		void Run();
	public:
		virtual void OnUpdate() = 0;
	protected:
		Ref<Window> m_Window;
	};
}