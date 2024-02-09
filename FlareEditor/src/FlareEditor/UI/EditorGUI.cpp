#include "EditorGUI.h"

#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Scene.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/UI/QuickSearch/QuickSearch.h"
#include "FlareEditor/UI/SerializablePropertyRenderer.h"

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_internal.h>

namespace Flare
{
    static EditorIcons s_EditorIcons(48);

    void EditorGUI::Initialize()
    {
        s_EditorIcons.Initialize();
    }

    bool EditorGUI::BeginPropertyGrid(float width)
    {
        float windowWidth = width == 0.0f ? ImGui::GetContentRegionAvail().x : width;
        if (ImGui::BeginTable("Property Grid", 2))
        {
            const float nameColumnWidthPercetage = 0.3f;

            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, windowWidth * nameColumnWidthPercetage);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, windowWidth * (1.0f - nameColumnWidthPercetage));
            return true;
        }
        return false;
    }

    void EditorGUI::EndPropertyGrid()
    {
        ImGui::EndTable();
    }

    void EditorGUI::DrawIcon(glm::ivec2 iconPosition, float size)
    {
        if (size == 0.0f)
        {
            const float defaultIconSize = 32.0f;
            size = defaultIconSize;
        }

        ImRect uvs = s_EditorIcons.GetIconUVs(iconPosition);
        ImGui::Image(s_EditorIcons.GetTexture()->GetRendererId(), ImVec2(size, size), uvs.Min, uvs.Max);
    }

    const EditorIcons& EditorGUI::GetIcons()
    {
        return s_EditorIcons;
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

    bool EditorGUI::IntVector2PropertyField(const char* name, glm::ivec2& value)
    {
        PropertyName(name);

        ImGui::PushID(&value);
        bool result = ImGui::DragInt2("", glm::value_ptr(value));
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

    bool EditorGUI::ObjectField(const SerializableObjectDescriptor& descriptor, void* data, const World* currentWorld)
    {
        bool result = false;

        FLARE_CORE_ASSERT(descriptor.Callback);

        SerializablePropertyRenderer propertyRenderer(currentWorld);
        propertyRenderer.PropertyKey(descriptor.Name);
        propertyRenderer.SerializeObject(descriptor, data, false, 0);

        // TODO: get the result from properties renderer
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

    inline static float CalculateAssetPreviewSize()
    {
        const ImGuiStyle& style = ImGui::GetStyle();

        float buttonHeight = style.FramePadding.y * 2.0f + ImGui::GetFontSize();
        return buttonHeight * 2.0f + style.ItemSpacing.y;
    }

    static void RenderAssetPreview(ImVec2 previewSize, ImVec2 previewPosition, bool hovered, AssetHandle handle)
    {
        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
        const ImGuiStyle& style = ImGui::GetStyle();

        ImU32 previewHoverColor = 0xffcccccc;
        const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);
        if (AssetManager::IsAssetHandleValid(handle))
        {
            Ref<Texture> previewTexture = nullptr;
            ImVec2 uvMin = ImVec2(0.0f, 1.0f);
            ImVec2 uvMax = ImVec2(1.0f, 0.0f);

            if (metadata->Type == AssetType::Texture)
            {
                previewTexture = AssetManager::GetAsset<Texture>(handle);
            }
            else if (metadata->Type == AssetType::Sprite)
            {
                Ref<Sprite> sprite = AssetManager::GetAsset<Sprite>(handle);
                previewTexture = sprite->GetTexture();

                // NOTE: Flipped vertically
                uvMin = ImVec2(sprite->UVMin.x, sprite->UVMax.y);
                uvMax = ImVec2(sprite->UVMax.x, sprite->UVMin.y);
            }

            if (previewTexture)
            {
                drawList->AddImageRounded((ImTextureID)previewTexture->GetRendererId(),
                    previewPosition, previewPosition + previewSize,
                    uvMin, uvMax,
                    hovered ? previewHoverColor : 0xffffffff,
                    style.FrameRounding);
            }
            else
            {
                // Empty preview
                drawList->AddRect(
                    previewPosition, previewPosition + previewSize,
                    hovered ? previewHoverColor : 0xffffffff,
                    style.FrameRounding);
            }
        }
        else
        {
            std::string_view noneText = "None";
            ImVec2 textSize = ImGui::CalcTextSize(noneText.data(), noneText.data() + noneText.size());

            ImVec2 textPosition = previewPosition + previewSize / 2.0f - textSize / 2.0f;
            drawList->AddText(textPosition,
                ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]),
                noneText.data(),
                noneText.data() + noneText.size());
        }
    }

    bool EditorGUI::AssetField(AssetHandle& handle, const AssetDescriptor* assetType)
    {
        bool result = false;
        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
        const ImGuiStyle& style = ImGui::GetStyle();

        const float previewSize = CalculateAssetPreviewSize();
        const float buttonHeight = style.FramePadding.y * 2.0f + ImGui::GetFontSize();
        const ImVec2 controlButtonSize = ImVec2(buttonHeight - 4.0f, buttonHeight - 4.0f);

        const float availableContentWidth = ImGui::GetContentRegionAvail().x;
        const float previewAndControlsWidth = previewSize + style.ItemSpacing.x + controlButtonSize.x;

        const bool validHandle = AssetManager::IsAssetHandleValid(handle);

        const ImVec2 initialCursorPosition = ImGui::GetCursorPos();
        const float previewXPosition = initialCursorPosition.x;
        const float nameXPosition = initialCursorPosition.x + previewSize + style.ItemSpacing.x;

        const AssetMetadata* metadata = validHandle ? AssetManager::GetAssetMetadata(handle) : nullptr;

        // Preview
        
        ImGui::PushID(&handle);
        if (ImGui::InvisibleButton("", ImVec2(previewSize, previewSize)))
            EditorLayer::GetInstance().Selection.SetAsset(handle);
        ImGui::PopID();

        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            if (handle == NULL_ASSET_HANDLE)
                ImGui::TextUnformatted("Handle: NULL");
            else if (validHandle)
                ImGui::Text("Handle: %llu", (uint64_t)handle);
            else
                ImGui::TextUnformatted("Handle: Invalid");

            ImGui::EndTooltip();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_PAYLOAD_NAME))
            {
                AssetHandle newHandle = *(AssetHandle*)payload->Data;
                Ref<Asset> asset = AssetManager::GetRawAsset(newHandle);
                
                if (!asset)
                {
                    handle = NULL_ASSET_HANDLE;
                    result = true;
                }
                else if (&asset->GetDescriptor() == assetType || assetType == nullptr)
                {
                    handle = newHandle;
                    result = true;
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImVec2 previewMin = ImGui::GetItemRectMin();
        ImVec2 previewMax = previewMin + ImVec2(previewSize, previewSize);

        bool hovered = ImGui::IsItemHovered();
        RenderAssetPreview(previewMax - previewMin, previewMin, hovered, handle);

        drawList->AddRect(previewMin,
            previewMax,
            ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]),
            style.FrameRounding);

        // Asset name
        
        float maxTextWidth = availableContentWidth - style.ItemSpacing.x * 2.0f - previewAndControlsWidth;
        ImGui::SetCursorPosX(nameXPosition);
        ImGui::SetCursorPosY(initialCursorPosition.y + previewSize / 2.0f - buttonHeight / 2.0f);

        bool showSearchWindow = false;
        if (validHandle)
        {
            showSearchWindow = ImGui::Button(metadata->Name.c_str(), ImVec2(maxTextWidth, buttonHeight));
        }
        else
        {
            ImGui::PushID(&handle);
            showSearchWindow = ImGui::Button("", ImVec2(maxTextWidth, buttonHeight));
            ImGui::PopID();
        }

        // Draw control button

        ImGui::SetCursorPos(ImVec2(
            initialCursorPosition.x + availableContentWidth - controlButtonSize.x - style.ItemSpacing.x,
            initialCursorPosition.y + previewSize / 2.0f - controlButtonSize.y / 2.0f));

        if (ImGui::InvisibleButton("Reset", controlButtonSize))
        {
            handle = NULL_ASSET_HANDLE;
            result = true;
        }

        ImRect resetButtonRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
        ImRect iconUVs = s_EditorIcons.GetIconUVs(EditorIcons::CloseIcon);

        ImU32 hoverColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);

        drawList->AddImage((ImTextureID)s_EditorIcons.GetTexture()->GetRendererId(),
            resetButtonRect.Min,
            resetButtonRect.Max,
            iconUVs.Min,
            iconUVs.Max,
            ImGui::IsItemHovered() ? hoverColor : 0xffffffff);

        UUID resultToken = (uint64_t)&handle;
        if (showSearchWindow)
        {
            QuickSearch::GetInstance().FindAsset(resultToken);
        }

        if (auto searchResult = QuickSearch::GetInstance().AcceptAssetResult(resultToken))
        {
            if (searchResult->Type == QuickSearch::AssetSearchResult::ResultType::Ok)
            {
                handle = searchResult->Handle;
                result = true;
            }
        }

        return result;
    }

    bool EditorGUI::AssetField(const char* name, AssetHandle& handle, const AssetDescriptor* assetType)
    {
        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
        const ImGuiStyle& style = ImGui::GetStyle();

        float buttonHeight = style.FramePadding.y * 2.0f + ImGui::GetFontSize();
        float previewSize = CalculateAssetPreviewSize();

        PropertyName(name, previewSize);
        return AssetField(handle, assetType);
    }
}