#include "Application.h"

#include "Flare/Renderer/RenderCommand.h"

namespace Flare
{
	Application::Application()
		: m_Running(true)
	{
		WindowProperties properties;
		properties.Title = "Flare Engine";
		properties.Width = 1280;
		properties.Height = 720;

		m_Window = Window::Create(properties);
		m_Window->SetEventCallback([this](Event& event)
		{
			EventDispatcher dispatcher(event);
			dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) -> bool
			{
				Close();
				return true;
			});

			dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) -> bool
			{
				RenderCommand::SetViewport(0, 0, event.GetWidth(), event.GetHeight());
				return true;
			});

			OnEvent(event);
		});

		RenderCommand::Initialize();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			float currentTime = Time::GetTime();
			float deltaTime = currentTime - m_PreviousFrameTime;

			OnUpdate(deltaTime);
			m_Window->OnUpdate();

			m_PreviousFrameTime = currentTime;
		}
	}

	void Application::Close()
	{
		m_Running = false;
	}
}