#include "WindowsWindow.h"

#include <iostream>

namespace Flare
{
	WindowsWindow::WindowsWindow(WindowProperties& properties)
	{
		m_Data.Properties = properties;

		Initialize();
	}

	void WindowsWindow::Initialize()
	{
		{
			int result = glfwInit();
			if (result == 0)
			{
				std::cout << "Failed to initialize glfw\n";
				return;
			}
		}

		m_Window = glfwCreateWindow(m_Data.Properties.Width, m_Data.Properties.Height, m_Data.Properties.Title.c_str(), nullptr, nullptr);

		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, (void*) &m_Data);
	}


	bool WindowsWindow::ShouldClose()
	{
		return glfwWindowShouldClose(m_Window);
	}

	WindowProperties& WindowsWindow::GetProperties()
	{
		return m_Data.Properties;
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	void WindowsWindow::Release()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();

		m_Window = nullptr;
	}
}