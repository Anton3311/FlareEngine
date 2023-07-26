#include "EditorTitleBar.h"

#include "Flare/Core/Application.h"
#include "Flare/Core/Window.h"
#include "Flare/Platform/Platform.h"
#include "Flare/Project/Project.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include <imgui_internal.h>

namespace Flare
{
	void EditorTitleBar::OnRenderImGui()
	{
		Ref<Window> window = Application::GetInstance().GetWindow();

		if (window->GetProperties().CustomTitleBar)
		{
			WindowControls& controls = window->GetWindowControls();

			if (controls.BeginTitleBar())
			{
				RenderTitleBar();

				controls.RenderControls();
				controls.EndTitleBar();
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
			ImGui::BeginDisabled(EditorContext::Instance.Mode != EditorMode::Edit);
			if (ImGui::MenuItem("Save"))
				Project::Save();

			if (ImGui::MenuItem("Open"))
			{
				std::optional<std::filesystem::path> projectPath = Platform::ShowOpenFileDialog(L"Flare Project (*.flareproj)\0*.flareproj\0");

				if (projectPath.has_value())
					Project::OpenProject(projectPath.value());
			}
			ImGui::EndDisabled();

			EditorGUI::EndMenu();
		}

		if (EditorGUI::BeginMenu("Scene"))
		{
			ImGui::BeginDisabled(EditorContext::Instance.Mode != EditorMode::Edit);
			if (ImGui::MenuItem("Save"))
				EditorLayer::GetInstance().SaveActiveScene();
			if (ImGui::MenuItem("Save As"))
				EditorLayer::GetInstance().SaveActiveSceneAs();

			ImGui::EndDisabled();
			EditorGUI::EndMenu();
		}

		window->DC.LayoutType = prevLayout;
	}
}
