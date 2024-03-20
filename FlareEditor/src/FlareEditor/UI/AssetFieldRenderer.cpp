#include "AssetFieldRenderer.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

namespace Flare
{
    inline static float CalculateAssetPreviewSize()
    {
        const ImGuiStyle& style = ImGui::GetStyle();

        float buttonHeight = style.FramePadding.y * 2.0f + ImGui::GetFontSize();
        return buttonHeight * 2.0f + style.ItemSpacing.y;
    }

	bool AssetFieldRenderer::OnRenderImGui()
	{
        GetValidHandle();

        bool result = false;
        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
        const ImGuiStyle& style = ImGui::GetStyle();

        const float previewSize = CalculateAssetPreviewSize();
        const float buttonHeight = style.FramePadding.y * 2.0f + ImGui::GetFontSize();
        const ImVec2 controlButtonSize = ImVec2(buttonHeight - 4.0f, buttonHeight - 4.0f);

        const float availableContentWidth = ImGui::GetContentRegionAvail().x;
        const float previewAndControlsWidth = previewSize + style.ItemSpacing.x + controlButtonSize.x;

        const ImVec2 initialCursorPosition = ImGui::GetCursorPos();
        const float previewXPosition = initialCursorPosition.x;
        const float nameXPosition = initialCursorPosition.x + previewSize + style.ItemSpacing.x;

        m_AssetMetadata = m_ValidHandle
            ? AssetManager::GetAssetMetadata(*m_ValidHandle)
            : nullptr;

        ImGui::PushID((int32_t)GetNextWidgetId());
        if (ImGui::InvisibleButton("", ImVec2(previewSize, previewSize)))
        {
            if (m_Handle != nullptr)
				EditorLayer::GetInstance().Selection.SetAsset(*m_Handle);
        }
        ImGui::PopID();

        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            bool isNullHandle = false;
            if (m_Asset && *m_Asset)
                isNullHandle = m_Asset->get()->Handle == NULL_ASSET_HANDLE;
            else if (m_ValidHandle)
                isNullHandle = *m_ValidHandle == NULL_ASSET_HANDLE;

			if (isNullHandle)
				ImGui::TextUnformatted("Handle: NULL");
            else if (m_ValidHandle)
				ImGui::Text("Handle: %llu", (uint64_t)*m_ValidHandle);
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

                result = SetAssetResult(asset);
            }

            ImGui::EndDragDropTarget();
        }

        ImVec2 previewMin = ImGui::GetItemRectMin();
        ImVec2 previewMax = previewMin + ImVec2(previewSize, previewSize);

        bool hovered = ImGui::IsItemHovered();
        RenderAssetPreview(previewMax - previewMin, previewMin, hovered);

        drawList->AddRect(previewMin,
            previewMax,
            ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]),
            style.FrameRounding);

        // Asset name
        
        float maxTextWidth = availableContentWidth - style.ItemSpacing.x * 2.0f - previewAndControlsWidth;
        ImGui::SetCursorPosX(nameXPosition);
        ImGui::SetCursorPosY(initialCursorPosition.y + previewSize / 2.0f - buttonHeight / 2.0f);

        bool showSearchWindow = false;
        if (m_ValidHandle)
        {
            showSearchWindow = ImGui::Button(m_AssetMetadata->Name.c_str(), ImVec2(maxTextWidth, buttonHeight));
        }
        else
        {
            ImGui::PushID((int32_t)GetNextWidgetId());
            showSearchWindow = ImGui::Button("", ImVec2(maxTextWidth, buttonHeight));
            ImGui::PopID();
        }

        // Draw control button

        ImGui::SetCursorPos(ImVec2(
            initialCursorPosition.x + availableContentWidth - controlButtonSize.x - style.ItemSpacing.x,
            initialCursorPosition.y + previewSize / 2.0f - controlButtonSize.y / 2.0f));

        ImGui::PushID((int32_t)GetNextWidgetId());
        if (ImGui::InvisibleButton("Reset", controlButtonSize))
        {
            ResetValue();
            result = true;
        }
        ImGui::PopID();

        ImRect resetButtonRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
        ImRect iconUVs = EditorGUI::GetIcons().GetIconUVs(EditorIcons::CloseIcon);

        ImU32 hoverColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);

        drawList->AddImage(ImGuiLayer::GetId(EditorGUI::GetIcons().GetTexture()),
            resetButtonRect.Min,
            resetButtonRect.Max,
            iconUVs.Min,
            iconUVs.Max,
            ImGui::IsItemHovered() ? hoverColor : 0xffffffff);

        UUID resultToken = GetNextWidgetId();
        if (showSearchWindow)
        {
            QuickSearch::GetInstance().FindAsset(resultToken);
        }

        if (auto searchResult = QuickSearch::GetInstance().AcceptAssetResult(resultToken))
        {
            if (searchResult->Type == QuickSearch::AssetSearchResult::ResultType::Ok)
            {
                Ref<Asset> rawAsset = AssetManager::GetRawAsset(searchResult->Handle);
                SetAssetResult(rawAsset);
                result = true;
            }
        }

        return result;
	}

    uint64_t AssetFieldRenderer::GetNextWidgetId()
    {
        return m_WidgetId++;
    }

    void AssetFieldRenderer::RenderAssetPreview(ImVec2 previewSize, ImVec2 previewPosition, bool hovered)
    {
        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
        const ImGuiStyle& style = ImGui::GetStyle();

        ImU32 previewHoverColor = 0xffcccccc;

        bool valid = false;
		Ref<Texture> previewTexture = nullptr;

		ImVec2 uvMin = ImVec2(0.0f, 1.0f);
		ImVec2 uvMax = ImVec2(1.0f, 0.0f);
        if (m_Asset && *m_Asset)
        {
            if (m_AssetDescriptor == &Texture::_Asset)
            {
                previewTexture = As<Texture>(*m_Asset);
                valid = true;
            }
            else if (m_AssetDescriptor == &Sprite::_Asset)
            {
                Ref<Sprite> sprite = As<Sprite>(*m_Asset);
                previewTexture = sprite->GetTexture();

                uvMin = ImVec2(sprite->UVMin.x, sprite->UVMin.y);
                uvMax = ImVec2(sprite->UVMax.x, sprite->UVMax.y);
                std::swap(uvMin.y, uvMax.y); // Flip vertically
                valid = true;
            }
        }
        else if (m_Handle && m_AssetMetadata)
        {
            if (m_AssetMetadata->Type == AssetType::Texture)
            {
                previewTexture = AssetManager::GetAsset<Texture>(*m_Handle);
                valid = true;
            }
            else if (m_AssetMetadata->Type == AssetType::Sprite)
            {
                Ref<Sprite> sprite = AssetManager::GetAsset<Sprite>(*m_Handle);
                previewTexture = sprite->GetTexture();

                uvMin = ImVec2(sprite->UVMin.x, sprite->UVMin.y);
                uvMax = ImVec2(sprite->UVMax.x, sprite->UVMax.y);
                std::swap(uvMin.y, uvMax.y); // Flip vertically
                valid = true;
            }
        }

        if (valid)
        {
            if (previewTexture)
            {
                drawList->AddImageRounded(ImGuiLayer::GetId(previewTexture),
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

    void AssetFieldRenderer::GetValidHandle()
    {
        if (m_Handle != nullptr)
        {
			if (AssetManager::IsAssetHandleValid(*m_Handle))
                m_ValidHandle = *m_Handle;
        }
        else if (m_Asset != nullptr && *m_Asset)
        {
			AssetHandle handle = m_Asset->get()->Handle;
			if (AssetManager::IsAssetHandleValid(handle))
				m_ValidHandle = handle;
        }
    }

    bool AssetFieldRenderer::SetAssetResult(const Ref<Asset>& asset)
    {
        FLARE_CORE_ASSERT(asset);
        if (m_AssetDescriptor && m_AssetDescriptor != &asset->GetDescriptor())
			return false;

        if (m_Asset != nullptr)
        {
            *m_Asset = asset;
            return true;
        }

        if (m_Handle != nullptr)
        {
            *m_Handle = asset->Handle;
            return true;
        }

        FLARE_CORE_ASSERT(false);
        return false;
    }

    void AssetFieldRenderer::ResetValue()
    {
		if (m_Asset != nullptr)
			*m_Asset = nullptr;
		else if (m_Handle != nullptr)
			*m_Handle = NULL_ASSET_HANDLE;
    }
}
