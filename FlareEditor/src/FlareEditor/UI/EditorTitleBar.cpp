#include "EditorTitleBar.h"

#include "Flare/Core/Application.h"
#include "Flare/Project/Project.h"

#include "FlarePlatform/Platform.h"
#include "FlarePlatform/Window.h"

#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/ProjectSettingsWindow.h"
#include "FlareEditor/UI/ECS/ECSInspector.h"

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

			const auto& viewports = EditorLayer::GetInstance().GetViewportWindows();
			for (const Ref<ViewportWindow>& viewportWindow : viewports)
			{
				if (ImGui::MenuItem(viewportWindow->GetName().c_str(), nullptr, viewportWindow->IsWindowVisible))
					viewportWindow->IsWindowVisible = !viewportWindow->IsWindowVisible;
			}

			EditorGUI::EndMenu();
		}

		float buttonHeight = window->MenuBarHeight();

		ImVec2 buttonSize = ImVec2(60.0f, buttonHeight);
		if (EditorLayer::GetInstance().GetMode() == EditorMode::Edit)
		{
			if (ImGui::Button("Play", buttonSize))
				EditorLayer::GetInstance().EnterPlayMode();
		}
		else
		{
			if (ImGui::Button("Stop", buttonSize))
				EditorLayer::GetInstance().ExitPlayMode();

			// TODO: replace with icons
			if (!EditorLayer::GetInstance().IsPlaymodePaused())
			{
				if (ImGui::Button("Pause"))
					EditorLayer::GetInstance().SetPlaymodePaused(true);
			}
			else
			{
				if (ImGui::Button("Continue"))
					EditorLayer::GetInstance().SetPlaymodePaused(false);
			}
		}

		window->DC.LayoutType = prevLayout;
	}
}
