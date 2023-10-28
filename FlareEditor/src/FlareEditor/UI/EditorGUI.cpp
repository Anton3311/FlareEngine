#include "EditorGUI.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Scene.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/UI/QuickSearch/QuickSearch.h"

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

	bool EditorGUI::BoolPropertyField(const char* name, bool& value)
	{
		PropertyName(name);

		ImGui::PushID(&value);
		bool result = ImGui::Checkbox("", &value);
		ImGui::PopID();

		ImGui::PopItemWidth();
		return result;
	}

	bool EditorGUI::IntPropertyField(const char* name, int32_t& value)
	{
		PropertyName(name);

		ImGui::PushID(&value);
		bool result = ImGui::DragInt("", &value, 0.1f);
		ImGui::PopID();

		ImGui::PopItemWidth();
		return result;
	}

	bool EditorGUI::UIntPropertyField(const char* name, uint32_t& value)
	{
		PropertyName(name);

		ImGui::PushID(&value);
		bool result = ImGui::DragScalar("", ImGuiDataType_U32, &value);
		ImGui::PopID();

		ImGui::PopItemWidth();
		return result;
	}

	bool EditorGUI::FloatPropertyField(const char* name, float& value)
	{
		PropertyName(name);

		ImGui::PushID(name);
		bool result = ImGui::DragFloat("", &value, 0.1f);
		ImGui::PopID();

		ImGui::PopItemWidth();
		return result;
	}

	bool EditorGUI::Vector2PropertyField(const char* name, glm::vec2& value)
	{
		PropertyName(name);

		ImGui::PushID(name);
		bool result = ImGui::DragFloat2("", glm::value_ptr(value), 0.1f);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::Vector3PropertyField(const char* name, glm::vec3& value)
	{
		PropertyName(name);

		ImGui::PushID(name);
		bool result = ImGui::DragFloat3("", glm::value_ptr(value), 0.1f);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::Vector4PropertyField(const char* name, glm::vec4& value)
	{
		PropertyName(name);

		ImGui::PushID(name);
		bool result = ImGui::DragFloat4("", glm::value_ptr(value), 0.1f);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::ColorPropertyField(const char* name, glm::vec4& color)
	{
		PropertyName(name);

		ImGui::PushID(&color);
		bool result = ImGui::ColorEdit4("", glm::value_ptr(color), ImGuiColorEditFlags_Float);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::AssetField(const char* name, AssetHandle& handle)
	{
		PropertyName(name);
		return AssetField(handle);
	}

	static int32_t InputTextCallback(ImGuiInputTextCallbackData* data)
	{
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			std::string* str = (std::string*)data->UserData;
			FLARE_CORE_ASSERT(data->Buf == str->c_str());

			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();

			return 1;
		}

		return 0;
	}

	bool EditorGUI::TextProperty(const char* name, std::string& text)
	{
		PropertyName(name);

		ImGui::PushID(&text);
 		bool result = ImGui::InputTextMultiline("", text.data(), text.size(), ImVec2(0.0f, 0.0f), ImGuiInputTextFlags_CallbackResize, InputTextCallback, (void*)&text);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::TextField(const char* name, std::string& text)
	{
		float y = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(y + ImGui::GetStyle().FramePadding.y);

		ImGui::Text(name);
		ImGui::SameLine();

		ImGui::SetCursorPosY(y);

		return TextField((uint64_t)name, text);
	}

	bool EditorGUI::TextField(UUID id, std::string& text)
	{
		ImGui::PushID(id);
		bool result = ImGui::InputText("",
			text.data(),
			text.size(),
			ImGuiInputTextFlags_CallbackResize,
			InputTextCallback,
			(void*)&text);

		ImGui::PopID();
		return result;
	}

	bool EditorGUI::EntityField(const char* name, const World& world, Entity& entity)
	{
		PropertyName(name);
		return EntityField(world, entity);
	}

	bool EditorGUI::AssetField(AssetHandle& handle)
	{
		ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x - 120.0f, 0);

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

		{
			ImGui::SameLine();

			UUID resultToken = (uint64_t)&handle;
			if (ImGui::Button("Find", ImVec2(45.0f, 0.0f)))
				QuickSearch::GetInstance().FindAsset(resultToken);

			if (auto searchResult = QuickSearch::GetInstance().AcceptAssetResult(resultToken))
			{
				if (searchResult->Type == QuickSearch::AssetSearchResult::ResultType::Ok)
				{
					handle = searchResult->Handle;
					result = true;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
			{
				handle = NULL_ASSET_HANDLE;
				return true;
			}
		}

		return result;
	}

	bool EditorGUI::EntityField(const World& world, Entity& entity)
	{
		bool alive = world.IsEntityAlive(entity);
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
		PropertyName(name);
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

	static bool RenderFieldEditor(const FieldTypeInfo& field, uint8_t* fieldData)
	{
		bool result = false;
		ImGui::PushID(fieldData);
		if (field.FieldType == SerializableFieldType::Custom)
		{
			if (field.CustomType == &Entity::_Type)
				result = EditorGUI::EntityField(World::GetCurrent(), *(Entity*)fieldData);
			else if (field.CustomType == &AssetHandle::_Type)
				result = EditorGUI::AssetField(*(AssetHandle*)fieldData);

			ImGui::PopID();
			return result;
		}

		switch (field.FieldType)
		{
		case SerializableFieldType::Bool:
			result |= ImGui::Checkbox("", (bool*)fieldData);
			break;
		case SerializableFieldType::Int32:
			result |= ImGui::DragInt("", (int32_t*)fieldData);
			break;
		case SerializableFieldType::Float32:
			result |= ImGui::DragFloat("", (float*)fieldData);
			break;

		case SerializableFieldType::Int2:
			result |= ImGui::DragInt2("", (int32_t*)fieldData);
			break;
		case SerializableFieldType::Int3:
			result |= ImGui::DragInt3("", (int32_t*)fieldData);
			break;
		case SerializableFieldType::Int4:
			result |= ImGui::DragInt4("", (int32_t*)fieldData);
			break;

		case SerializableFieldType::Float2:
			result |= ImGui::DragFloat2("", (float*)fieldData);
			break;
		case SerializableFieldType::Float3:
			result |= ImGui::DragFloat3("", (float*)fieldData);
			break;
		case SerializableFieldType::Float4:
			result |= ImGui::DragFloat4("", (float*)fieldData);
			break;
		case SerializableFieldType::String:
		{
			std::string* string = (std::string*)fieldData;
			result |= ImGui::InputTextMultiline("", string->data(), string->size() + 1, ImVec2(0.0f, 0.0f), ImGuiInputTextFlags_CallbackResize, InputTextCallback, (void*)string);
			break;
		}
		}

		ImGui::PopID();
		return result;
	}

	static bool RenderArrayEditor(const FieldData& field, uint8_t* data)
	{
		bool result = false;
		EditorGUI::EndPropertyGrid();

		bool opened = ImGui::TreeNodeEx(field.Name, ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth);
		if (opened)
		{
			EditorGUI::BeginPropertyGrid();
			float framePadding = ImGui::GetStyle().FramePadding.y;
			size_t elementSize = field.ArrayElementType.GetSize();

			{
				// Array Size
				EditorGUI::PropertyName("Size");

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + framePadding);
				ImGui::Text("%d", field.ElementsCount);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + framePadding);
			}

			for (size_t i = 0; i < field.ElementsCount; i++)
			{
				EditorGUI::PropertyIndex(i);
				result |= RenderFieldEditor(field.ArrayElementType, data + i * elementSize);
			}

			EditorGUI::EndPropertyGrid();
			ImGui::TreePop();
		}

		EditorGUI::BeginPropertyGrid();
		return result;
	}

	bool EditorGUI::TypeEditor(const TypeInitializer& type, uint8_t* data)
	{
		const auto& fields = type.SerializedFields;
		bool result = false;

		if (EditorGUI::BeginPropertyGrid())
		{
			for (size_t i = 0; i < fields.size(); i++)
			{
				const FieldData& field = fields[i];
				uint8_t* fieldData = data + field.Offset;

				if (field.TypeInfo.FieldType == SerializableFieldType::Array)
					result |= RenderArrayEditor(field, fieldData);
				else if (field.TypeInfo.FieldType == SerializableFieldType::Custom && field.TypeInfo.CustomType != &Entity::_Type && field.TypeInfo.CustomType != &AssetHandle::_Type)
				{
					EditorGUI::EndPropertyGrid();
					bool opened = ImGui::TreeNodeEx(field.Name, ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth);
					if (opened)
					{
						result |= EditorGUI::TypeEditor(*field.TypeInfo.CustomType, fieldData);
						ImGui::TreePop();
					}
					EditorGUI::BeginPropertyGrid();
				}
				else
				{
					PropertyName(field.Name);
					result |= RenderFieldEditor(field.TypeInfo, data + field.Offset);
				}
			}
			EditorGUI::EndPropertyGrid();
		}

		return result;
	}

	void EditorGUI::PropertyName(const char* name)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
		ImGui::Text(name);

		ImGui::TableSetColumnIndex(1);
		ImGui::PushItemWidth(-1);
	}

	void EditorGUI::PropertyIndex(size_t index)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
		ImGui::Text("%d", index);

		ImGui::TableSetColumnIndex(1);
		ImGui::PushItemWidth(-1);
	}
}