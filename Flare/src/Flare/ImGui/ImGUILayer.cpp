#include "ImGUILayer.h"

#include "Flare/Core/Application.h"
#include "Flare/Core/Core.h"

#ifdef FLARE_PLATFORM_WINDOWS
	#include "Flare/Platform/Windows/WindowsWindow.h"
#endif

#include <stdint.h>

#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

namespace Flare
{
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

		ImFont* roboto = io.Fonts->AddFontFromFileTTF("assets/Fonts/Roboto/Roboto-Regular.ttf", 14.0f);
		io.FontDefault = roboto;

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
		ImGuizmo::BeginFrame();
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
		style.WindowBorderSize = 1.0f;
		style.WindowMenuButtonPosition = ImGuiDir_None;

		style.FrameRounding = 4.0f;
		style.FrameBorderSize = 0.0f;
		style.FramePadding = ImVec2(6.0f, 6.0f);

		style.GrabRounding = 4.0f;

		style.TabRounding = 4.0f;
		style.TabBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;

		style.PopupRounding = 0.0f;
		style.PopupBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;

		auto& colors = style.Colors;

		colors[ImGuiCol_Text] = ImGuiTheme::Text;
		colors[ImGuiCol_TextDisabled] = ImGuiTheme::TextDisabled;
		colors[ImGuiCol_TextSelectedBg] = ImGuiTheme::TextSelectionBackground;

		colors[ImGuiCol_WindowBg] = ImGuiTheme::WindowBackground;
		colors[ImGuiCol_PopupBg] = ImGuiTheme::WindowBackground;
		colors[ImGuiCol_ChildBg] = ImGuiTheme::WindowBackground;
		colors[ImGuiCol_Border] = ImGuiTheme::WindowBorder;
		
		colors[ImGuiCol_FrameBg] = ImGuiTheme::FrameBackground;
		colors[ImGuiCol_FrameBgActive] = ImGuiTheme::FrameActiveBackground;
		colors[ImGuiCol_FrameBgHovered] = ImGuiTheme::FrameHoveredBackground;

		colors[ImGuiCol_TitleBg] = ImGuiTheme::WindowBackground;
		colors[ImGuiCol_TitleBgActive] = ImGuiTheme::WindowBackground;
		colors[ImGuiCol_TitleBgCollapsed] = ImGuiTheme::WindowBackground;

		colors[ImGuiCol_Tab] = ImGuiTheme::Surface;
		colors[ImGuiCol_TabHovered] = ImGuiTheme::PrimaryVariant;
		colors[ImGuiCol_TabActive] = ImGuiTheme::Primary;
		colors[ImGuiCol_TabUnfocused] = ImGuiTheme::Surface;
		colors[ImGuiCol_TabUnfocusedActive] = ImGuiTheme::Surface;

		colors[ImGuiCol_ScrollbarBg] = ImGuiTheme::WindowBackground;
		colors[ImGuiCol_SliderGrab] = ImGuiTheme::Primary;
		colors[ImGuiCol_SliderGrabActive] = ImGuiTheme::Primary;

		colors[ImGuiCol_Separator] = ImGuiTheme::WindowBorder;
		colors[ImGuiCol_SeparatorActive] = ImGuiTheme::Primary;
		colors[ImGuiCol_SeparatorHovered] = ImGuiTheme::Primary;

		colors[ImGuiCol_ResizeGripHovered] = ImGuiTheme::PrimaryVariant;
		colors[ImGuiCol_ResizeGripActive] = ImGuiTheme::Primary;

		colors[ImGuiCol_Button] = ImGuiTheme::FrameBackground;
		colors[ImGuiCol_ButtonHovered] = ImGuiTheme::FrameHoveredBackground;
		colors[ImGuiCol_ButtonActive] = ImGuiTheme::FrameActiveBackground;

		colors[ImGuiCol_Header] = ImGuiTheme::FrameBackground;
		colors[ImGuiCol_HeaderHovered] = ImGuiTheme::FrameHoveredBackground;
		colors[ImGuiCol_HeaderActive] = ImGuiTheme::FrameActiveBackground;

		colors[ImGuiCol_CheckMark] = ImGuiTheme::Primary;

		ImGuizmo::Style& guizmoStyle = ImGuizmo::GetStyle();
		guizmoStyle.RotationLineThickness = 4.0f;
		guizmoStyle.TranslationLineThickness = 4.0f;
	}
}