#pragma once

#include <Flare/Core/Window.h>
#include <Flare/Renderer/GraphicsContext.h>

#include <GLFW/glfw3.h>

namespace Flare
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(WindowProperties& properties);
	public:
		virtual const WindowProperties& GetProperties() const override { return m_Data.Properties; }

		virtual void SetTitle(const std::string& title) override;
		virtual void Hide() override;
		virtual void SetMaximized(bool value) override;

		virtual void SetEventCallback(const EventCallback& callback) override { m_Data.Callback = callback; }
		virtual void SetVSync(bool vsync) override;

		virtual void* GetNativeWindow() override { return m_Window; }

		virtual void OnUpdate() override;

		GLFWwindow* GetGLFWWindow() const { return m_Window; }

		virtual WindowControls& GetWindowControls() override { return m_Data.Controls; }
		virtual glm::uvec2 GetControlsButtonSize() override;
	private:
		void Initialize();
		void Release();

		static LRESULT CALLBACK CustomWindowDecorationProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
		static const wchar_t* s_WindowPropertyName;
	private:
		struct WindowData
		{
			WindowProperties Properties;
			EventCallback Callback;
			WindowControls Controls;
		};

		GLFWwindow* m_Window;
		Scope<GraphicsContext> m_GraphicsContext;
		WindowData m_Data;

		void* m_OriginalProc;
	};
}