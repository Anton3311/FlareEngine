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

		virtual void SetEventCallback(const EventCallback& callback) override { m_Data.Callback = callback; }
		virtual void SetVSync(bool vsync) override;

		virtual void OnUpdate() override;
	private:
		void Initialize();
		void Release();
	private:
		struct WindowData
		{
			WindowProperties Properties;
			EventCallback Callback;
		};

		GLFWwindow* m_Window;
		Scope<GraphicsContext> m_GraphicsContext;
		WindowData m_Data;
	};
}