#include "ImGuiLayer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Core/Application.h"
#include "Flare/Renderer/RendererAPI.h"

#include "FlarePlatform/Window.h"

#include "FlareEditor/ImGui/ImGuiLayerVulkan.h"

#include <imgui_internal.h>
#include <ImGuizmo.h>

namespace Flare
{
	ImVec4 ColorFromHex(uint32_t hex)
	{
		uint8_t r = (uint8_t)((hex & 0xff000000) >> 24);
		uint8_t g = (uint8_t)((hex & 0x00ff0000) >> 16);
		uint8_t b = (uint8_t)((hex & 0x0000ff00) >> 8);
		uint8_t a = (uint8_t)((hex & 0x000000ff) >> 0);

		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}

	ImVec4 ImGuiTheme::Text = ColorFromHex(0xffffffff);
	ImVec4 ImGuiTheme::TextDisabled = ColorFromHex(0x8A8F98ff);
	ImVec4 ImGuiTheme::TextSelectionBackground = ColorFromHex(0x4E9F3Dff); // Same as primary

	ImVec4 ImGuiTheme::WindowBackground = ColorFromHex(0x191A19ff);
	ImVec4 ImGuiTheme::WindowBorder = ColorFromHex(0x505250ff);

	ImVec4 ImGuiTheme::FrameBorder = ColorFromHex(0x505250ff);
	ImVec4 ImGuiTheme::FrameBackground = ColorFromHex(0x383938ff);
	ImVec4 ImGuiTheme::FrameHoveredBackground = ColorFromHex(0x585958ff);
	ImVec4 ImGuiTheme::FrameActiveBackground = ColorFromHex(0x4C4E4Cff);

	ImVec4 ImGuiTheme::Primary = ColorFromHex(0x4E9F3Dff);
	ImVec4 ImGuiTheme::PrimaryVariant = ColorFromHex(0x256917ff);

	ImVec4 ImGuiTheme::Surface = ColorFromHex(0x383938ff);

	Ref<ImGuiLayer> s_Instance = nullptr;

	void ImGuiLayer::OnAttach()
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

		InitializeRenderer();

		ImFont* roboto = io.Fonts->AddFontFromFileTTF("assets/Fonts/Roboto/Roboto-Regular.ttf", 14.0f);
		io.FontDefault = roboto;

		SetThemeColors();

		InitializeFonts();
	}

	void ImGuiLayer::OnDetach()
	{
		ShutdownRenderer();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::End()
	{
	}

	void ImGuiLayer::BeginDockSpace()
	{
        static bool fullscreen = true;
        static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
        if (fullscreen)
        {
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
            windowFlags |= ImGuiWindowFlags_NoBackground;

        static bool open = true;
        ImGui::Begin("DockSpace", &open, windowFlags);

        ImGuiID dockspaceId = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
	}

	void ImGuiLayer::EndDockSpace()
	{
        ImGui::End();
	}

	ImTextureID ImGuiLayer::GetId(const Ref<const Texture>& texture)
	{
		return s_Instance->GetTextureId(texture);
	}

	ImTextureID ImGuiLayer::GetId(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachmentIndex)
	{
		return s_Instance->GetFrameBufferAttachmentId(frameBuffer, attachmentIndex);
	}

	void ImGuiLayer::SetThemeColors()
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
		ImGuizmo::SetGizmoSizeClipSpace(0.12f);
	}

	Ref<ImGuiLayer> ImGuiLayer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			s_Instance = CreateRef<ImGuiLayerVulkan>();
			break;
		}

		return s_Instance;
	}
}