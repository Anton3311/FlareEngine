#include "WindowsWindow.h"

#include "Flare.h"
#include "Flare/Core/Assert.h"

#include <windows.h>
#include <WinUser.h>
#include <dwmapi.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Flare
{
	const wchar_t* WindowsWindow::s_WindowPropertyName = L"FlareWin";

	static KeyCode TranslateGLFWKey(int key)
	{
		switch (key)
		{
			case GLFW_KEY_SPACE: return KeyCode::Space;
			case GLFW_KEY_APOSTROPHE : return KeyCode::Apostrophe;
			case GLFW_KEY_COMMA: return KeyCode::Comma;
			case GLFW_KEY_MINUS: return KeyCode::Minus;
			case GLFW_KEY_PERIOD: return KeyCode::Period;
			case GLFW_KEY_SLASH: return KeyCode::Slash;
			case GLFW_KEY_0: return KeyCode::D0;
			case GLFW_KEY_1: return KeyCode::D1;
			case GLFW_KEY_2: return KeyCode::D2;
			case GLFW_KEY_3: return KeyCode::D3;
			case GLFW_KEY_4: return KeyCode::D4;
			case GLFW_KEY_5: return KeyCode::D5;
			case GLFW_KEY_6: return KeyCode::D6;
			case GLFW_KEY_7: return KeyCode::D7;
			case GLFW_KEY_8: return KeyCode::D8;
			case GLFW_KEY_9: return KeyCode::D9;
			case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
			case GLFW_KEY_EQUAL: return KeyCode::Equal;
			case GLFW_KEY_A: return KeyCode::A;
			case GLFW_KEY_B: return KeyCode::B;
			case GLFW_KEY_C: return KeyCode::C;
			case GLFW_KEY_D: return KeyCode::D;
			case GLFW_KEY_E: return KeyCode::E;
			case GLFW_KEY_F: return KeyCode::F;
			case GLFW_KEY_G: return KeyCode::G;
			case GLFW_KEY_H: return KeyCode::H;
			case GLFW_KEY_I: return KeyCode::I;
			case GLFW_KEY_J: return KeyCode::J;
			case GLFW_KEY_K: return KeyCode::K;
			case GLFW_KEY_L: return KeyCode::L;
			case GLFW_KEY_M: return KeyCode::M;
			case GLFW_KEY_N: return KeyCode::N;
			case GLFW_KEY_O: return KeyCode::O;
			case GLFW_KEY_P: return KeyCode::P;
			case GLFW_KEY_Q: return KeyCode::Q;
			case GLFW_KEY_R: return KeyCode::R;
			case GLFW_KEY_S: return KeyCode::S;
			case GLFW_KEY_T: return KeyCode::T;
			case GLFW_KEY_U: return KeyCode::U;
			case GLFW_KEY_V: return KeyCode::V;
			case GLFW_KEY_W: return KeyCode::W;
			case GLFW_KEY_X: return KeyCode::X;
			case GLFW_KEY_Y: return KeyCode::Y;
			case GLFW_KEY_Z: return KeyCode::Z;
			case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
			case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
			case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
			case GLFW_KEY_GRAVE_ACCENT: return KeyCode::GraveAccent;
			case GLFW_KEY_WORLD_1: return KeyCode::World1;
			case GLFW_KEY_WORLD_2: return KeyCode::World2;

			case GLFW_KEY_ESCAPE: return KeyCode::Escape;
			case GLFW_KEY_ENTER: return KeyCode::Enter;
			case GLFW_KEY_TAB: return KeyCode::Tab;
			case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
			case GLFW_KEY_INSERT: return KeyCode::Insert;
			case GLFW_KEY_DELETE: return KeyCode::Delete;
			case GLFW_KEY_RIGHT: return KeyCode::Right;
			case GLFW_KEY_LEFT: return KeyCode::Left;
			case GLFW_KEY_DOWN: return KeyCode::Down;
			case GLFW_KEY_UP: return KeyCode::Up;
			case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
			case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
			case GLFW_KEY_HOME: return KeyCode::Home;
			case GLFW_KEY_END: return KeyCode::End;
			case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
			case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
			case GLFW_KEY_NUM_LOCK: return KeyCode::NumLock;
			case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
			case GLFW_KEY_PAUSE: return KeyCode::Pause;
			case GLFW_KEY_F1: return KeyCode::F1;
			case GLFW_KEY_F2: return KeyCode::F2;
			case GLFW_KEY_F3: return KeyCode::F3;
			case GLFW_KEY_F4: return KeyCode::F4;
			case GLFW_KEY_F5: return KeyCode::F5;
			case GLFW_KEY_F6: return KeyCode::F6;
			case GLFW_KEY_F7: return KeyCode::F7;
			case GLFW_KEY_F8: return KeyCode::F8;
			case GLFW_KEY_F9: return KeyCode::F9;
			case GLFW_KEY_F10: return KeyCode::F10;
			case GLFW_KEY_F11: return KeyCode::F11;
			case GLFW_KEY_F12: return KeyCode::F12;
			case GLFW_KEY_F13: return KeyCode::F13;
			case GLFW_KEY_F14: return KeyCode::F14;
			case GLFW_KEY_F15: return KeyCode::F15;
			case GLFW_KEY_F16: return KeyCode::F16;
			case GLFW_KEY_F17: return KeyCode::F17;
			case GLFW_KEY_F18: return KeyCode::F18;
			case GLFW_KEY_F19: return KeyCode::F19;
			case GLFW_KEY_F20: return KeyCode::F20;
			case GLFW_KEY_F21: return KeyCode::F21;
			case GLFW_KEY_F22: return KeyCode::F22;
			case GLFW_KEY_F23: return KeyCode::F23;
			case GLFW_KEY_F24: return KeyCode::F24;
			case GLFW_KEY_F25: return KeyCode::F25;
			case GLFW_KEY_KP_0: return KeyCode::KP0;
			case GLFW_KEY_KP_1: return KeyCode::KP1;
			case GLFW_KEY_KP_2: return KeyCode::KP2;
			case GLFW_KEY_KP_3: return KeyCode::KP3;
			case GLFW_KEY_KP_4: return KeyCode::KP4;
			case GLFW_KEY_KP_5: return KeyCode::KP5;
			case GLFW_KEY_KP_6: return KeyCode::KP6;
			case GLFW_KEY_KP_7: return KeyCode::KP7;
			case GLFW_KEY_KP_8: return KeyCode::KP8;
			case GLFW_KEY_KP_9: return KeyCode::KP9;
			case GLFW_KEY_KP_DECIMAL: return KeyCode::KPDecimal;
			case GLFW_KEY_KP_DIVIDE: return KeyCode::KPDivide;
			case GLFW_KEY_KP_MULTIPLY: return KeyCode::KPMultiply;
			case GLFW_KEY_KP_SUBTRACT: return KeyCode::KPSubtract;
			case GLFW_KEY_KP_ADD: return KeyCode::KPAdd;
			case GLFW_KEY_KP_ENTER: return KeyCode::KPEnter;
			case GLFW_KEY_KP_EQUAL: return KeyCode::KPEqual;
			case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
			case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
			case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
			case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
			case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
			case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
			case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
			case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;
			case GLFW_KEY_MENU: return KeyCode::Menu;
			default:
				FLARE_CORE_ASSERT("Unhandled key translation");
		}
	}

	LRESULT CALLBACK WindowsWindow::CustomWindowDecorationProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		WindowsWindow* window = (WindowsWindow*)GetPropW(windowHandle, WindowsWindow::s_WindowPropertyName);

		switch (message)
		{
		case WM_CREATE:
		{
			RECT size_rect;
			GetWindowRect(windowHandle, &size_rect);

			SetWindowPos(
				windowHandle, NULL,
				size_rect.left, size_rect.top,
				size_rect.right - size_rect.left, size_rect.bottom - size_rect.top,
				SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE
			);
			break;
		}
		case WM_ACTIVATE:
		{
			LONG_PTR lStyle = GetWindowLongPtr(windowHandle, GWL_STYLE);
			lStyle |= WS_THICKFRAME;
			lStyle &= ~WS_CAPTION;
			SetWindowLongPtr(windowHandle, GWL_STYLE, lStyle);

			RECT windowRect;
			GetWindowRect(windowHandle, &windowRect);
			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;
			SetWindowPos(windowHandle, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOCOPYBITS);
			RECT title_bar_rect = { 0 };
			InvalidateRect(windowHandle, &title_bar_rect, FALSE);

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
					GetDpiForWindow(windowHandle));
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
				const HMONITOR mh = MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST);

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
			GetWindowPlacement(windowHandle, &placement);

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
			ScreenToClient(windowHandle, &clientMousePos);

			RECT windowRect;
			GetClientRect(windowHandle, &windowRect);

			enum HitResult
			{
				HitNone = 0,
				HitLeft = 1,
				HitRight = 2,
				HitTop = 4,
				HitBottom = 8,
			};

			int32_t result = HitNone;
			if (clientMousePos.x <= borderWidth)
				result |= HitLeft;
			if (clientMousePos.y <= borderWidth)
				result |= HitTop;
			if (clientMousePos.x >= windowRect.right - borderWidth)
				result |= HitRight;
			if (clientMousePos.y >= windowRect.bottom - borderWidth)
				result |= HitBottom;

			if (result == HitNone && window->m_Data.Controls.IsTitleBatHovered())
				return HTCAPTION;

			switch (result)
			{
			case HitLeft | HitTop:
				return HTTOPLEFT;
			case HitLeft | HitBottom:
				return HTBOTTOMLEFT;
			case HitRight | HitTop:
				return HTTOPRIGHT;
			case HitRight | HitBottom:
				return HTBOTTOMRIGHT;
			case HitLeft:
				return HTLEFT;
			case HitRight:
				return HTRIGHT;
			case HitTop:
				return HTTOP;
			case HitBottom:
				return HTBOTTOM;
			}

			break;
		}
		}

		return CallWindowProc(window->m_OriginalProc, windowHandle, message, wParam, lParam);
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

	// From https://github.com/bitsdojo/bitsdojo_window
	//
	// see: https://github.com/bitsdojo/bitsdojo_window/blob/816d217e770303035494653bd80f67484ca159e7/bitsdojo_window_windows/lib/src/window.dart#L122
	glm::uvec2 WindowsWindow::GetControlsButtonSize()
	{
		uint32_t dpi = GetDpiForWindow(glfwGetWin32Window(m_Window));
		float scaleFactor = dpi / 96.0f;

		float titleBarHeight = GetSystemMetricsForDpi(SM_CYCAPTION, dpi) / scaleFactor;
		float sizeFrame = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) / scaleFactor;
		float paddedBorder = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi) / scaleFactor;

		float height = titleBarHeight + sizeFrame + paddedBorder;

		float caption = GetSystemMetricsForDpi(SM_CYCAPTION, dpi) / scaleFactor;
		float width = caption * 2;

		return glm::uvec2(width, height);
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

		glfwSetWindowMaximizeCallback(m_Window, [](GLFWwindow* window, int maximized)
		{
			WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
			data->Properties.IsMaximized = maximized;
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
			KeyCode translatedKey = TranslateGLFWKey(key);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(translatedKey);
					data->Callback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(translatedKey);
					data->Callback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(translatedKey, true);
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

		if (m_Data.Properties.CustomTitleBar)
		{
			HWND windowHandle = glfwGetWin32Window(m_Window);

			LONG_PTR style = GetWindowLongPtr(windowHandle, GWL_STYLE);
			style |= WS_THICKFRAME;
			style &= ~WS_CAPTION;
			SetWindowLongPtr(windowHandle, GWL_STYLE, style);

			RECT windowRect;
			GetWindowRect(windowHandle, &windowRect);
			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;

			MARGINS margins0 = { 0 };
			DwmExtendFrameIntoClientArea(windowHandle, &margins0);

			bool result = SetPropW(windowHandle, s_WindowPropertyName, this);
			FLARE_CORE_ASSERT(result, "Failed to set window property");

			m_OriginalProc = (WNDPROC)GetWindowLongPtr(windowHandle, GWLP_WNDPROC);
			SetWindowLongPtr(windowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CustomWindowDecorationProc));
			SetWindowPos(windowHandle, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOCOPYBITS);
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

	void WindowsWindow::Hide()
	{
		glfwIconifyWindow(m_Window);
	}

	void WindowsWindow::SetMaximized(bool value)
	{
		if (value)
			glfwMaximizeWindow(m_Window);
		else
			glfwRestoreWindow(m_Window);
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