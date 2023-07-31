#include "ProjectSettingsWindow.h"

#include "Flare/Project/Project.h"
#include "Flare/Project/ProjectSerializer.h"

#include "FlareEditor/UI/EditorGUI.h"

#include <imgui.h>
#include <glm/glm.hpp>

namespace Flare
{
	bool ProjectSettingsWindow::m_Opened = false;
	void ProjectSettingsWindow::OnRenderImGui()
	{
		if (!m_Opened)
			return;

		if (ImGui::Begin("Project Settings", &m_Opened))
		{
			ImVec2 windowSize = ImGui::GetContentRegionAvail();

			Ref<Project> project = Project::GetActive();

			if (EditorGUI::BeginPropertyGrid())
			{
				if (EditorGUI::AssetField("Start Scene", project->StartScene))
					Project::Save();

				EditorGUI::EndPropertyGrid();
			}

			ImGui::End();
		}
	}

	void ProjectSettingsWindow::Show()
	{
		m_Opened = true;
	}
}
