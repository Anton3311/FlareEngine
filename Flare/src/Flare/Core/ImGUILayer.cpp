#include "ImGUILayer.h"

#include "Flare/Core/Application.h"
#include "Flare/Core/Core.h"
#include "Flare/Core/Platform.h"

#ifdef FLARE_PLATFORM_WINDOWS
	#include "Flare/Platform/Windows/WindowsWindow.h"
#endif

#include <stdint.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

ImVec4 operator*(const ImVec4& vec, float scalar)
{
	return ImVec4(vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar);
}

namespace Flare
{
	static ImVec4 ColorFromHex(uint32_t hex)
	{
		uint8_t r = (uint8_t)((hex & 0xff000000) >> 24);
		uint8_t g = (uint8_t)((hex & 0x00ff0000) >> 16);
		uint8_t b = (uint8_t)((hex & 0x0000ff00) >> 8);
		uint8_t a = (uint8_t)((hex & 0x000000ff) >> 0);
	
		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}

	ImGUILayer::ImGUILayer()
		: Layer("ImGUILayer")
	{

	}

	void ImGUILayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		Application& application = Application::GetInstance();
		Ref<Window> window = application.GetWindow();

#ifdef FLARE_PLATFORM_WINDOWS
		ImGui_ImplGlfw_InitForOpenGL(((WindowsWindow*)window.get())->GetGLFWWindow(), true);
		ImGui_ImplOpenGL3_Init("#version 410");
#endif

		SetThemeColors();
	}

	void ImGUILayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGUILayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
	}

	void ImGUILayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& application = Application::GetInstance();

		const WindowProperties& windowProps = application.GetWindow()->GetProperties();
		io.DisplaySize = ImVec2(windowProps.Width, windowProps.Height);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* currentContext = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(currentContext);
		}
	}

	void ImGUILayer::SetThemeColors()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.FrameRounding = 4.0f;
		style.GrabRounding = 4.0f;

		style.TabRounding = 0.0f;
		style.TabBorderSize = 0.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.FrameBorderSize = 1.5f;
		style.PopupBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;

		style.WindowMenuButtonPosition = ImGuiDir_None;

		style.FramePadding = ImVec2(6.0f, 6.0f);

		auto& colors = style.Colors;

		ImVec4 windowBackgroundColor = ColorFromHex(0x2D3142ff);
		ImVec4 frameBorder = ColorFromHex(0x8D99AEFF) * 0.6f;
		ImVec4 primaryColor = ColorFromHex(0xF45D01FF);
		ImVec4 surfaceColor = ColorFromHex(0x434758FF);

		colors[ImGuiCol_WindowBg] = windowBackgroundColor;
		colors[ImGuiCol_PopupBg] = windowBackgroundColor;
		colors[ImGuiCol_ChildBg] = windowBackgroundColor;
		colors[ImGuiCol_Border] = frameBorder;

		colors[ImGuiCol_FrameBg] = ColorFromHex(0x1C2133ff);
		colors[ImGuiCol_FrameBgHovered] = ColorFromHex(0x282D41ff);

		colors[ImGuiCol_TitleBg] = windowBackgroundColor;
		colors[ImGuiCol_TitleBgActive] = windowBackgroundColor;
		colors[ImGuiCol_TitleBgCollapsed] = windowBackgroundColor;

		colors[ImGuiCol_Tab] = surfaceColor;
		colors[ImGuiCol_TabHovered] = primaryColor * 0.9f;
		colors[ImGuiCol_TabActive] = primaryColor;
		colors[ImGuiCol_TabUnfocused] = surfaceColor;
		colors[ImGuiCol_TabUnfocusedActive] = surfaceColor;

		colors[ImGuiCol_ScrollbarBg] = windowBackgroundColor;
		colors[ImGuiCol_SliderGrab] = primaryColor;
		colors[ImGuiCol_SliderGrabActive] = primaryColor * 0.9f;

		colors[ImGuiCol_Separator] = frameBorder;
		colors[ImGuiCol_SeparatorActive] = primaryColor * 0.9f;
		colors[ImGuiCol_SeparatorHovered] = primaryColor;

		colors[ImGuiCol_ResizeGripHovered] = primaryColor;
		colors[ImGuiCol_ResizeGripActive] = primaryColor * 0.9f;

		colors[ImGuiCol_Button] = surfaceColor;
		colors[ImGuiCol_ButtonHovered] = surfaceColor * 0.9f;
		colors[ImGuiCol_ButtonActive] = surfaceColor * 0.9f;

		colors[ImGuiCol_Header] = surfaceColor;
		colors[ImGuiCol_HeaderHovered] = surfaceColor * 0.9f;
		colors[ImGuiCol_HeaderActive] = surfaceColor * 0.9f;

		colors[ImGuiCol_CheckMark] = primaryColor;
	}
}