#pragma once

#include "Flare/Core/Window.h"

namespace Flare
{
	class Application
	{
	public:
		Application();

		void Run();
		void Close();
	public:
		virtual void OnUpdate(float deltaTime) = 0;

		virtual void OnEvent(Event& event) {}
	protected:
		Ref<Window> m_Window;
	private:
		bool m_Running;

		float m_PreviousFrameTime;
	};
}