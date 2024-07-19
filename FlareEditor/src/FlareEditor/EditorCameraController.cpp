#include "EditorCameraController.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	void EditorCameraController::OnWindowFocus()
	{
	}

	void EditorCameraController::Update(glm::vec2 mousePosition)
	{
		bool windowFocused = ImGui::IsWindowFocused();
		bool windowHovered = ImGui::IsWindowHovered();

		if (!m_IsMouseDown && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) // Just clicked
		{
			m_Camera.ResetPreviousMousePosition(mousePosition);
			m_IsMouseDown = true;
		}

		const auto& io = ImGui::GetIO();
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
			m_IsMouseDown = false;

		if (windowFocused && windowHovered && ImGui::IsMouseDown(ImGuiPopupFlags_MouseButtonMiddle))
		{
			bool move = ImGui::IsKeyDown(ImGuiKey_ModShift);
			ImVec2 mouseDelta = io.MouseDelta;

			if (move)
				m_Camera.Drag(mousePosition);
			else
				m_Camera.Rotate(glm::vec2(mouseDelta.x, mouseDelta.y));
		}

		if (windowHovered)
		{
			m_Camera.Zoom(-io.MouseWheel);
		}
	}
}