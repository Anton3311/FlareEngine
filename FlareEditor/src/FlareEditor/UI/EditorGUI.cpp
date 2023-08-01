#include "EditorGUI.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/EditorContext.h"

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_internal.h>

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

	bool EditorGUI::BeginMenu(const char* name)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		if (ImGui::Button(name))
		{
			ImVec2 buttonMin = ImGui::GetItemRectMin();
			ImVec2 buttonMax = ImGui::GetItemRectMax();
			ImGui::SetNextWindowPos(ImVec2(buttonMin.x, buttonMax.y));
			ImGui::OpenPopup(name);
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(1);
		
		return ImGui::BeginPopup(name, ImGuiWindowFlags_NoMove);
	}

	void EditorGUI::EndMenu()
	{
		ImGui::EndMenu();
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

		ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x - 60.0f, 0);

		if (handle == NULL_ASSET_HANDLE)
			ImGui::Button("None", buttonSize);
		else
		{
			const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);

			if (metadata != nullptr)
				ImGui::Button(metadata->Path.filename().string().c_str(), buttonSize);
			else
				ImGui::Button("Invalid", buttonSize);
		}

		bool result = false;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
			{
				handle = *(AssetHandle*)payload->Data;
				result = true;
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		if (ImGui::Button("Reset", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
		{
			handle = NULL_ASSET_HANDLE;
			return true;
		}

		return result;
	}

	bool EditorGUI::EntityField(const char* name, const World& world, Entity& entity)
	{
		bool alive = world.IsEntityAlive(entity);

		RenderPropertyName(name);

		float buttonWidth = ImGui::GetContentRegionAvail().x - 60.0f;

		if (alive)
			ImGui::Button(fmt::format("Enitty {0} {1}", entity.GetIndex(), entity.GetGeneration()).c_str(), ImVec2(buttonWidth, 0));
		else
			ImGui::Button("None", ImVec2(buttonWidth, 0));

		bool result = false;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ENTITY_PAYLOAD_NAME))
			{
				entity = *(Entity*)payload->Data;
				result = true;
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		if (ImGui::Button("Reset", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
		{
			entity = Entity();
			return true;
		}

		return result;
	}

	bool EditorGUI::BeginToggleGroup(const char* name, uint32_t itemsCount)
	{
		float itemWidth = ImGui::GetContentRegionAvail().x - (itemsCount) * ImGui::GetStyle().ItemInnerSpacing.x;
		itemWidth /= itemsCount;

		ImGui::PushItemWidth(itemWidth);
		ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Horizontal;
		return true;
	}

	bool EditorGUI::BeginToggleGroupProperty(const char* name, uint32_t itemsCount)
	{
		RenderPropertyName(name);
		return BeginToggleGroup(name, itemsCount);
	}

	bool EditorGUI::ToggleGroupItem(const char* text, bool selected)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_TabActive]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_TabHovered]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_TabActive]);
		}

		bool result = ImGui::Button(text, ImVec2(ImGui::CalcItemWidth(), 0));

		if (selected)
			ImGui::PopStyleColor(3);

		return result;
	}

	void EditorGUI::EndToggleGroup()
	{
		ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Vertical;
		ImGui::PopItemWidth();
	}

	bool EditorGUI::TypeEditor(const Internal::ScriptingType& type, uint8_t* data)
	{
		const auto& fields = type.GetSerializationSettings().GetFields();
		bool result = false;

		if (EditorGUI::BeginPropertyGrid())
		{
			for (size_t i = 0; i < fields.size(); i++)
			{
				const Internal::Field& field = fields[i];
				uint8_t* fieldData = data + field.Offset;

				switch (field.Type)
				{
				case Internal::FieldType::Float:
					result |= EditorGUI::FloatPropertyField(field.Name.c_str(), *(float*)fieldData);
					break;
				case Internal::FieldType::Float2:
					result |= EditorGUI::Vector2PropertyField(field.Name.c_str(), *(glm::vec2*)fieldData);
					break;
				case Internal::FieldType::Float3:
					result |= EditorGUI::Vector3PropertyField(field.Name.c_str(), *(glm::vec3*)fieldData);
					break;
				case Internal::FieldType::Asset:
				case Internal::FieldType::Texture:
					result |= EditorGUI::AssetField(field.Name.c_str(), *(AssetHandle*)fieldData);
					break;
				case Internal::FieldType::Entity:
					result |= EditorGUI::EntityField(field.Name.c_str(), Scene::GetActive()->GetECSWorld(), *(Entity*)fieldData);
					break;
				}
			}
			EditorGUI::EndPropertyGrid();
		}

		return result;
	}

	void EditorGUI::RenderPropertyName(const char* name)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
		ImGui::Text(name);

		ImGui::TableSetColumnIndex(1);
		ImGui::PushItemWidth(-1);
	}
}