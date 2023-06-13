#include "Application.h"

#include "Flare/Renderer/RenderCommand.h"

namespace Flare
{
	Application::Application()
	{
		WindowProperties properties;
		properties.Title = "Flare Engine";
		properties.Width = 1280;
		properties.Height = 720;

		m_Window = Window::Create(properties);

		RenderCommand::Initialize();
	}

	void Application::Run()
	{
		while (!m_Window->ShouldClose())
		{
			OnUpdate();
			m_Window->OnUpdate();
		}
	}
}