#include "EditorGUI.h"

#include <imgui.h>

namespace Flare
{
	bool EditorGUI::BeginPropertyGrid()
	{
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		return ImGui::BeginTable("sdf", 2);
	}

	void EditorGUI::EndPropertyGrid()
	{
		ImGui::EndTable();
	}

	bool EditorGUI::FloatPropertyField(const char* name, float& value)
	{
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::Text(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(-1);

		ImGui::PushID(&value);
		bool result = ImGui::DragFloat("", &value, 0.1f);
		ImGui::PopID();

		ImGui::PopItemWidth();
		return result;
	}
}