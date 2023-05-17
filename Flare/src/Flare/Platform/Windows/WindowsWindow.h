#pragma once

#include <Flare/Core/Window.h>

#include <GLFW/glfw3.h>

namespace Flare
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(WindowProperties& properties);
	public:
		virtual bool ShouldClose() override;
		virtual WindowProperties& GetProperties() override;

		virtual void OnUpdate() override;
	private:
		void Initialize();
		void Release();
	private:
		struct WindowData
		{
			WindowProperties Properties;
		};

		GLFWwindow* m_Window;
		WindowData m_Data;
	};
}