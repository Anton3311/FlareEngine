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

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event((KeyCode)key);
					data->Callback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event((KeyCode)key);
					data->Callback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event((KeyCode)key, true);
					data->Callback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keyCode)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event((KeyCode)keyCode);
			data->Callback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event((MouseCode)button);
					data->Callback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event((MouseCode)button);
					data->Callback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);

			MouseScrollEvent event(glm::vec2((float)x, (float)y));
			data->Callback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);

			MouseMoveEvent event(glm::vec2((float)x, (float)y));
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