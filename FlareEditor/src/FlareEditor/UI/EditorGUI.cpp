#include "EditorGUI.h"

#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Scene.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/UI/QuickSearch/QuickSearch.h"
#include "FlareEditor/UI/SerializablePropertyRenderer.h"

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

	void EditorGUI::MoveCursor(ImVec2 offset)
	{
		ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);
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

	bool EditorGUI::ObjectField(SerializableObject& object)
	{
		bool result = false;

		SerializablePropertyRenderer propertyRenderer;
		SerializationStream stream(propertyRenderer);

		propertyRenderer.PropertyKey(object.Descriptor.Name);
		propertyRenderer.BeginObject(&object.Descriptor);
		object.Descriptor.Callback(object.GetBuffer(), stream);
		propertyRenderer.EndObject();

		// TODO: get the result from properties renderer
		return result;
	}

	bool EditorGUI::PropertyField(SerializableProperty& property)
	{
		bool result = false;

		switch (property.Descriptor.PropertyType)
		{
		case SerializablePropertyType::Bool:
			result |= BoolPropertyField(property.Descriptor.Name.c_str(), property.ValueAs<bool>());
			break;

		case SerializablePropertyType::Float:
			result |= FloatPropertyField(property.Descriptor.Name.c_str(), property.ValueAs<float>());
			break;
		case SerializablePropertyType::Int32:
			result |= IntPropertyField(property.Descriptor.Name.c_str(), property.ValueAs<int32_t>());
			break;

		case SerializablePropertyType::FloatVector2:
			result |= Vector2PropertyField(property.Descriptor.Name.c_str(), property.ValueAs<glm::vec2>());
			break;
		case SerializablePropertyType::FloatVector3:
			result |= Vector3PropertyField(property.Descriptor.Name.c_str(), property.ValueAs<glm::vec3>());
			break;
		case SerializablePropertyType::FloatVector4:
			result |= Vector4PropertyField(property.Descriptor.Name.c_str(), property.ValueAs<glm::vec4>());
			break;

		case SerializablePropertyType::Texture2D:
			result |= TextureField(property.Descriptor.Name.c_str(), property.ValueAs<AssetHandle>());
			break;

		case SerializablePropertyType::String:
			result |= EditorGUI::TextProperty(property.Descriptor.Name.c_str(), property.ValueAs<std::string>());
			break;
		case SerializablePropertyType::UUID:
			EditorGUI::UUIDField(property.Descriptor.Name.c_str(), property.ValueAs<UUID>());
			break;

		case SerializablePropertyType::Color3:
			EditorGUI::ColorPropertyField(property.Descriptor.Name.c_str(), property.ValueAs<glm::vec3>());
			break;
		case SerializablePropertyType::Color4:
			EditorGUI::ColorPropertyField(property.Descriptor.Name.c_str(), property.ValueAs<glm::vec4>());
			break;
		}
		
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

	bool EditorGUI::ColorPropertyField(const char* name, glm::vec3& color)
	{
		PropertyName(name);

		ImGui::PushID(&color);
		bool result = ImGui::ColorEdit3("", glm::value_ptr(color), ImGuiColorEditFlags_Float);
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
		return TextField(name, text);
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

	bool EditorGUI::TextField(std::string& text)
	{
		ImGui::PushID(&text);
 		bool result = ImGui::InputTextMultiline("", text.data(), text.size(), ImVec2(0.0f, 0.0f), ImGuiInputTextFlags_CallbackResize, InputTextCallback, (void*)&text);
		ImGui::PopID();
		return result;
	}

	bool EditorGUI::TextField(UUID id, std::string& text)
	{
		ImGui::PushID((int32_t)(uint64_t)id);
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
			{
				if (metadata->Name.empty())
					ImGui::Button(metadata->Path.filename().string().c_str(), buttonSize);
				else
					ImGui::Button(metadata->Name.c_str(), buttonSize);
			}
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

	void EditorGUI::PropertyName(const char* name, float minHeight)
	{
		ImGui::TableNextRow(0, minHeight);
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

	void EditorGUI::UUIDField(const char* name, UUID uuid)
	{
		PropertyName(name);

		float y = ImGui::GetCursorPosY();

		ImGui::SetCursorPosY(y + ImGui::GetStyle().FramePadding.y);

		ImGui::Text("%llu", (uint64_t)uuid);
	}

	bool EditorGUI::TextureField(const char* name, AssetHandle& textureHandle)
	{
		ImVec2 previewSize = ImVec2(48.0f, 48.0f);

		bool result = false;
		PropertyName(name, previewSize.y);

		if (ImGui::Button("Reset"))
		{
			textureHandle = NULL_ASSET_HANDLE;
			result = true;
		}

		ImGui::SameLine();

		UUID resultToken = (uint64_t)&textureHandle;

		if (ImGui::InvisibleButton(name, previewSize))
			QuickSearch::GetInstance().FindAsset(resultToken);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
			{
				textureHandle = *(AssetHandle*)payload->Data;
				result = true;
			}

			ImGui::EndDragDropTarget();
		}

		ImVec2 previewMin = ImGui::GetItemRectMin();
		ImVec2 previewMax = previewMin + previewSize;

		if (auto searchResult = QuickSearch::GetInstance().AcceptAssetResult(resultToken))
		{
			if (searchResult->Type == QuickSearch::AssetSearchResult::ResultType::Ok)
			{
				textureHandle = searchResult->Handle;
				result = true;
			}
		}

		bool hovered = ImGui::IsItemHovered();

		ImU32 textureHoverColor = 0xffcccccc;

		ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
		const ImGuiStyle& style = ImGui::GetStyle();

		if (AssetManager::IsAssetHandleValid(textureHandle))
		{
			Ref<Texture> texture = AssetManager::GetAsset<Texture>(textureHandle);
			
			drawList->AddImageRounded((ImTextureID)texture->GetRendererId(),
				previewMin, previewMax,
				ImVec2(0.0f, 1.0f),
				ImVec2(1.0f, 0.0f),
				hovered ? textureHoverColor : 0xffffffff,
				style.FrameRounding);

		}
		else
		{
			std::string_view noneText = "None";
			ImVec2 textSize = ImGui::CalcTextSize(noneText.data(), noneText.data() + noneText.size());

			ImVec2 textPosition = previewMin + previewSize / 2.0f - textSize / 2.0f;
			drawList->AddText(textPosition,
				ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]),
				noneText.data(),
				noneText.data() + noneText.size());
		}

		drawList->AddRect(previewMin,
			previewMax,
			ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]),
			style.FrameRounding);

		return result;
	}
}