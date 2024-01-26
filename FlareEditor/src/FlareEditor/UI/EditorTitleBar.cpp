#include "EditorTitleBar.h"

#include "Flare/Core/Application.h"
#include "Flare/Project/Project.h"

#include "FlarePlatform/Platform.h"
#include "FlarePlatform/Window.h"

#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/ProjectSettingsWindow.h"
#include "FlareEditor/UI/ECS/ECSInspector.h"

#include "FlareEditor/UI/Profiler/ProfilerWindow.h"

#include <imgui_internal.h>

namespace Flare
{
	EditorTitleBar::EditorTitleBar()
	{
		m_WindowControls = CreateRef<WindowsWindowControls>();

		Application::GetInstance().GetWindow()->SetWindowControls(m_WindowControls);
	}

	void EditorTitleBar::OnRenderImGui()
	{
		Ref<Window> window = Application::GetInstance().GetWindow();

		if (window->GetProperties().CustomTitleBar)
		{
			if (m_WindowControls->BeginTitleBar())
			{
				RenderTitleBar();

				m_WindowControls->RenderControls();
				m_WindowControls->EndTitleBar();
			}
		}
	}

	void EditorTitleBar::RenderTitleBar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		auto prevLayout = window->DC.LayoutType;

		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		ImGui::SetCursorPos(ImVec2(8.0f, 8.0f));

		if (EditorGUI::BeginMenu("Project"))
		{
			ImGui::BeginDisabled(EditorLayer::GetInstance().GetMode() != EditorMode::Edit);
			if (ImGui::MenuItem("Save"))
				Project::Save();

			if (ImGui::MenuItem("Open"))
			{
				std::optional<std::filesystem::path> projectPath = Platform::ShowOpenFileDialog(
					L"Flare Project (*.flareproj)\0*.flareproj\0",
					Application::GetInstance().GetWindow());

				if (projectPath.has_value())
					Project::OpenProject(projectPath.value());
			}

			if (ImGui::MenuItem("Add Package"))
			{
				std::optional<std::filesystem::path> packagePath = Platform::ShowOpenFileDialog(
					L"Flare package (*.yaml)\0*.yaml\0",
					Application::GetInstance().GetWindow());

				if (packagePath.has_value())
					As<EditorAssetManager>(AssetManager::GetInstance())->AddAssetsPackage(packagePath.value());
			}

			if (ImGui::MenuItem("Settings"))
				ProjectSettingsWindow::Show();

			ImGui::EndDisabled();

			EditorGUI::EndMenu();
		}

		if (EditorGUI::BeginMenu("Scene"))
		{
			ImGui::BeginDisabled(EditorLayer::GetInstance().GetMode() != EditorMode::Edit);
			if (ImGui::MenuItem("New"))
				EditorLayer::GetInstance().CreateNewScene();
			if (ImGui::MenuItem("Save"))
				EditorLayer::GetInstance().SaveActiveScene();
			if (ImGui::MenuItem("Save As"))
				EditorLayer::GetInstance().SaveActiveSceneAs();

			ImGui::EndDisabled();
			EditorGUI::EndMenu();
		}

		if (EditorGUI::BeginMenu("Scripting"))
		{
			if (ImGui::MenuItem("Reload"))
			{
				EditorLayer::GetInstance().ReloadScriptingModules();
			}

			EditorGUI::EndMenu();
		}

		if (EditorGUI::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("ECS Inspector"))
				ECSInspector::Show();
			if (ImGui::MenuItem("Profiler"))
				ProfilerWindow::GetInstance().ShowWindow();

			const auto& viewports = EditorLayer::GetInstance().GetViewportWindows();
			for (const Ref<ViewportWindow>& viewportWindow : viewports)
			{
				if (ImGui::MenuItem(viewportWindow->GetName().c_str(), nullptr, viewportWindow->ShowWindow))
					viewportWindow->ShowWindow = !viewportWindow->ShowWindow;
			}

			EditorGUI::EndMenu();
		}

		const auto& style = ImGui::GetStyle();
		float buttonSize = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

		float buttonsWidth = buttonSize * 2.0f + style.ItemSpacing.x;

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f - buttonsWidth / 2.0f);

		if (EditorLayer::GetInstance().GetMode() == EditorMode::Edit)
		{
			if (RenderButton("Play", EditorIcons::PlayIcon))
				EditorLayer::GetInstance().EnterPlayMode();
		}
		else
		{
			if (RenderButton("Stop", EditorIcons::StopIcon))
				EditorLayer::GetInstance().ExitPlayMode();

			if (!EditorLayer::GetInstance().IsPlaymodePaused())
			{
				if (RenderButton("Pause", EditorIcons::PauseIcon))
					EditorLayer::GetInstance().SetPlaymodePaused(true);
			}
			else
			{
				if (RenderButton("Continue", EditorIcons::ContinueIcon))
					EditorLayer::GetInstance().SetPlaymodePaused(false);
			}
		}

		window->DC.LayoutType = prevLayout;
	}

	bool EditorTitleBar::RenderButton(const char* id, glm::ivec2 iconPosition)
	{
		const auto& style = ImGui::GetStyle();
		const auto& icons = EditorGUI::GetIcons();
		float buttonSize = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

		const ImU32 buttonHoverColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]);
		const ImU32 buttonActiveColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonActive]);
		const ImU32 buttonColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Button]);

		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

		ImGui::InvisibleButton(id, ImVec2(buttonSize, buttonSize));

		ImU32 color = buttonColor;
		if (ImGui::IsItemHovered())
			color = buttonHoverColor;

		if (ImGui::IsItemActive())
			color = buttonActiveColor;

		ImRect buttonRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
		drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color, style.FrameRounding);

		ImRect iconUVs = icons.GetIconUVs(iconPosition);
		drawList->AddImage(
			icons.GetTexture()->GetRendererId(),
			buttonRect.Min,
			buttonRect.Max,
			iconUVs.Min, iconUVs.Max);

		return ImGui::IsItemClicked();
	}
}
