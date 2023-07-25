#include "WindowsWindow.h"

#include "Flare.h"

#include <windows.h>
#include <WinUser.h>
#include <dwmapi.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Flare
{
	WNDPROC originalProc;

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CREATE:
		{
			RECT size_rect;
			GetWindowRect(hWnd, &size_rect);

			SetWindowPos(
				hWnd, NULL,
				size_rect.left, size_rect.top,
				size_rect.right - size_rect.left, size_rect.bottom - size_rect.top,
				SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE
			);
			break;
		}
		case WM_ACTIVATE:
		{
			LONG_PTR lStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			lStyle |= WS_THICKFRAME;
			lStyle &= ~WS_CAPTION;
			SetWindowLongPtr(hWnd, GWL_STYLE, lStyle);

			RECT windowRect;
			GetWindowRect(hWnd, &windowRect);
			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;
			SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOCOPYBITS);
			RECT title_bar_rect = { 0 };
			InvalidateRect(hWnd, &title_bar_rect, FALSE);

			return 0;
		}
		case WM_NCACTIVATE:
		{
			return TRUE;
		}
		case WM_GETMINMAXINFO:
		{
			// From GLFW win32_window.c
			
			RECT frame = { 0 };
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			const DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX;
			const DWORD exStyle = WS_EX_APPWINDOW;

			// TODO: check for windows version
			if (/*_glfwIsWindows10Version1607OrGreaterWin32()*/ true)
			{
				AdjustWindowRectExForDpi(&frame, style, FALSE, exStyle,
					GetDpiForWindow(hWnd));
			}
			else
				AdjustWindowRectEx(&frame, style, FALSE, exStyle);

			// TODO: support for min/max window size

			/*if (window->minwidth != GLFW_DONT_CARE &&
				window->minheight != GLFW_DONT_CARE)
			{
				mmi->ptMinTrackSize.x = window->minwidth + frame.right - frame.left;
				mmi->ptMinTrackSize.y = window->minheight + frame.bottom - frame.top;
			}

			if (window->maxwidth != GLFW_DONT_CARE &&
				window->maxheight != GLFW_DONT_CARE)
			{
				mmi->ptMaxTrackSize.x = window->maxwidth + frame.right - frame.left;
				mmi->ptMaxTrackSize.y = window->maxheight + frame.bottom - frame.top;
			}*/

			{
				MONITORINFO mi;
				const HMONITOR mh = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

				ZeroMemory(&mi, sizeof(mi));
				mi.cbSize = sizeof(mi);
				GetMonitorInfoW(mh, &mi);

				mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
				mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
				mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
				mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
			}

			return 0;
		}
		case WM_NCCALCSIZE:
		{
			WINDOWPLACEMENT placement;
			GetWindowPlacement(hWnd, &placement);

			if (placement.showCmd == SW_MAXIMIZE)
				return WVR_ALIGNTOP | WVR_ALIGNLEFT;

			if (wParam == TRUE && lParam != NULL)
			{
				int32_t resizeBorderX = GetSystemMetrics(SM_CXFRAME);
				int32_t resizeBorderY = GetSystemMetrics(SM_CYFRAME);

				NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
				pParams->rgrc[0].top += resizeBorderY * 0;
				pParams->rgrc[0].right -= resizeBorderX;
				pParams->rgrc[0].bottom -= resizeBorderY;
				pParams->rgrc[0].left += resizeBorderX;
			}
			return WVR_ALIGNTOP | WVR_ALIGNLEFT;
		}
		case WM_NCPAINT:
		{
			return 0;
		}
		case WM_NCHITTEST:
		{
			int32_t captionHeight = GetSystemMetrics(SM_CYCAPTION);
			int32_t borderWidth = GetSystemMetrics(SM_CXFRAME);

			POINTS mousePos = MAKEPOINTS(lParam);
			POINT clientMousePos = { mousePos.x, mousePos.y };
			ScreenToClient(hWnd, &clientMousePos);

			RECT windowRect;
			GetClientRect(hWnd, &windowRect);

			if (clientMousePos.y >= windowRect.bottom - borderWidth)
			{
				if (clientMousePos.x <= borderWidth)
					return HTBOTTOMLEFT;
				else if (clientMousePos.x >= windowRect.right - borderWidth)
					return HTBOTTOMRIGHT;
				else
					return HTBOTTOM;
			}
			else if (clientMousePos.y <= borderWidth)
			{
				if (clientMousePos.x <= borderWidth)
					return HTTOPLEFT;
				else if (clientMousePos.x >= windowRect.right - borderWidth)
					return HTTOPRIGHT;
				else
					return HTTOP;
			}
			else if (clientMousePos.x <= borderWidth)
			{
				return HTLEFT;
			}
			else if (clientMousePos.x >= windowRect.right - borderWidth)
			{
				return HTRIGHT;
			}

			if (clientMousePos.y >= 0 && clientMousePos.y <= captionHeight)
				return HTCAPTION;

			break;
		}
		}

		return CallWindowProc(originalProc, hWnd, uMsg, wParam, lParam);
	}

	float Time::GetTime()
	{
		return (float) glfwGetTime();
	}

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

		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

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

			if (data->Callback)
			{
				WindowResizeEvent event(width, height);
				data->Callback(event);
			}
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

		SetVSync(true);

		bool custom = true;
		if (custom)
		{
			HWND hWnd = glfwGetWin32Window(m_Window);

			LONG_PTR lStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			lStyle |= WS_THICKFRAME;
			lStyle &= ~WS_CAPTION;
			SetWindowLongPtr(hWnd, GWL_STYLE, lStyle);

			RECT windowRect;
			GetWindowRect(hWnd, &windowRect);
			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;

			MARGINS margins0 = { 0 };

			DwmExtendFrameIntoClientArea(hWnd, &margins0);

			originalProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc));
			SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOCOPYBITS);
		}
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

	void WindowsWindow::SetTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_Window, title.c_str());
		m_Data.Properties.Title = title;
	}

	void WindowsWindow::SetVSync(bool vsync)
	{
		m_Data.Properties.VSyncEnabled = vsync;

		if (vsync)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);
	}
}