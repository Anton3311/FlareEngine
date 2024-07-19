#pragma once

#include "FlareCore/Core.h"
#include "FlarePlatform/Window.h"

#include <GLFW/glfw3.h>

#include <Windows.h>

namespace Flare
{
	class FLAREPLATFORM_API WindowsWindow : public Window
	{
	public:
		WindowsWindow(WindowProperties& properties);
		virtual ~WindowsWindow();
	public:
		virtual const WindowProperties& GetProperties() const override { return m_Data.Properties; }

		void SetUsesVulkan();
		
		virtual void Initialize() override;

		virtual void SetTitle(const std::string& title) override;
		virtual void Hide() override;
		virtual void SetMaximized(bool value) override;

		virtual void SetFullscreenMode(FullscreenMode mode) override;

		virtual void SetEventCallback(const EventCallback& callback) override { m_Data.Callback = callback; }
		virtual void SetVSync(bool vsync) override;

		virtual void* GetNativeWindow() override { return m_Window; }

		virtual void OnUpdate() override;

		GLFWwindow* GetGLFWWindow() const { return m_Window; }

		virtual void SetWindowControls(const Ref<WindowControls>& controls) { m_Data.Controls = controls; }
		virtual Ref<WindowControls> GetWindowControls() const override { return m_Data.Controls; }
		virtual glm::uvec2 GetControlsButtonSize() const override;

		virtual void SetCursorMode(CursorMode mode) override;
	private:
		static LRESULT CALLBACK CustomWindowDecorationProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
		static const wchar_t* s_WindowPropertyName;

		void EnableWindowDecoration();
		void DisableWindowDecoration();

		void EnterFullscreen(FullscreenMode mode);
		void ExitFullscreen();
	private:
		enum class RendererAPI
		{
			Vulkan,
		};

		struct WindowData
		{
			WindowProperties Properties;
			EventCallback Callback = nullptr;
			Ref<WindowControls> Controls = nullptr;
		};

		RendererAPI m_RendererAPI = RendererAPI::Vulkan;

		glm::ivec2 m_LastWindowedPosition = glm::ivec2(0, 0);
		glm::uvec2 m_LastWindowedSize = glm::uvec2(0, 0);

		GLFWwindow* m_Window = nullptr;
		WindowData m_Data;

		void* m_OriginalProc = nullptr;
	};
}