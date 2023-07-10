#include "EditorGUI.h"

#include "Flare/AssetManager/AssetManager.h"

#include <imgui.h>

namespace Flare
{
	bool EditorGUI::BeginPropertyGrid()
	{
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		if (ImGui::BeginTable("Property Grid", 2))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, windowSize.x * 0.25f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, windowSize.x * 0.75f);
			return true;
		}
		return false;
	}

	void EditorGUI::EndPropertyGrid()
	{
		ImGui::EndTable();
	}

	bool EditorGUI::FloatPropertyField(const char* name, float& value)
	{
		RenderPropertyName(name);

		ImGui::PushID(&value);
		bool result = ImGui::DragFloat("", &value, 0.1f);
		ImGui::PopID();

		ImGui::PopItemWidth();
		return result;
	}

	bool EditorGUI::Vector2PropertyField(const char* name, glm::vec2& value)
	{
		RenderPropertyName(name);

		ImGui::PushID(&value);
		bool result = ImGui::DragFloat2("", glm::value_ptr(value), 0.1f);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::Vector3PropertyField(const char* name, glm::vec3& value)
	{
		RenderPropertyName(name);

		ImGui::PushID(&value);
		bool result = ImGui::DragFloat3("", glm::value_ptr(value), 0.1f);
		ImGui::PopID();
		return result;
	}
	bool EditorGUI::ColorPropertyField(const char* name, glm::vec4& color)
	{
		RenderPropertyName(name);

		ImGui::PushID(&color);
		bool result = ImGui::ColorEdit4("", glm::value_ptr(color), ImGuiColorEditFlags_Float);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::AssetField(const char* name, AssetHandle& handle)
	{
		RenderPropertyName(name);

		if (handle == NULL_ASSET_HANDLE)
			ImGui::Button("None");
		else
		{
			const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);

			if (metadata != nullptr)
				ImGui::Button(metadata->Path.filename().string().c_str());
			else
				ImGui::Button("Invalid");
		}

		bool result = false;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_HANDLE"))
			{
				handle = *(AssetHandle*)payload->Data;
				result = true;
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		if (ImGui::Button("Reset"))
		{
			handle = NULL_ASSET_HANDLE;
			return true;
		}

		return result;
	}

	bool EditorGUI::BeginToggleGroup(const char* name)
	{
		RenderPropertyName(name);

		ImGui::NewLine();
		return true;
	}

	bool EditorGUI::ToggleGroupItem(const char* text, bool selected)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		if (selected)
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_FrameBg]);
		else
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

		ImGui::SameLine();
		bool result = ImGui::Button(text);

		if (selected)
			ImGui::PopStyleColor();
		else
			ImGui::PopStyleVar();

		return result;
	}

	void EditorGUI::EndToggleGroup()
	{
	}

	void EditorGUI::RenderPropertyName(const char* name)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::Text(name);

		ImGui::TableSetColumnIndex(1);
		ImGui::PushItemWidth(-1);
	}
}