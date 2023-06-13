#include "WindowsWindow.h"

#include "Flare.h"

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
				FLARE_CORE_CRITICAL("Failed to initialize GLFW");
				return;
			}
		}

		m_Window = glfwCreateWindow(m_Data.Properties.Width, m_Data.Properties.Height, m_Data.Properties.Title.c_str(), nullptr, nullptr);

		m_GraphicsContext = GraphicsContext::Create((void*) m_Window);
		m_GraphicsContext->Initialize();

		glfwSetWindowUserPointer(m_Window, (void*) &m_Data);

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
			
			WindowCloseEvent event;
			data->Callback(event);
		});

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
			data->Properties.Width = width;
			data->Properties.Height = height;
			data->Properties.IsMinimized = width == 0 && height == 0;
			
			WindowResizeEvent event(width, height);
			data->Callback(event);
		});
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		m_GraphicsContext->SwapBuffers();
	}

	void WindowsWindow::Release()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();

		m_Window = nullptr;
	}
}