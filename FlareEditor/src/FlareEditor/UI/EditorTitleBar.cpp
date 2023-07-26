#include "EditorTitleBar.h"

#include "Flare/Core/Application.h"
#include "Flare/Core/Window.h"

#include <imgui_internal.h>

namespace Flare
{
	void EditorTitleBar::OnRenderImGui()
	{
		Ref<Window> window = Application::GetInstance().GetWindow();

		if (window->GetProperties().CustomTitleBar)
		{
			WindowControls& controls = window->GetWindowControls();

			controls.BeginTitleBar();

			RenderTitleBar();

			controls.RenderControls();
			controls.EndTitleBar();
		}
	}

	void EditorTitleBar::RenderTitleBar()
	{
		ImGui::SameLine();
	}
}
