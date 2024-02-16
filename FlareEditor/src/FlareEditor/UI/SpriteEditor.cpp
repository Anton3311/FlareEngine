#include "SpriteEditor.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/AssetManager/SpriteImporter.h"

#include <glm/glm.hpp>

namespace Flare
{
    FLARE_IMPL_ENUM_BITFIELD(SpriteEditor::SelectionRectSide);

    void SpriteEditor::RenderWindowContent()
    {
        RenderViewport();
        ImGui::SameLine();
        RenderSidebar();
    }

    void SpriteEditor::RenderViewport()
    {
        ImVec2 availableAreaSize = ImGui::GetContentRegionAvail();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysHorizontalScrollbar
            | ImGuiWindowFlags_AlwaysVerticalScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoMove;

        const auto& style = ImGui::GetStyle();
        const ImVec4 windowBackground = style.Colors[ImGuiCol_WindowBg];
        const float viewportBackgroundBrightness = 0.5f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(
            windowBackground.x * viewportBackgroundBrightness,
            windowBackground.y * viewportBackgroundBrightness,
            windowBackground.z * viewportBackgroundBrightness,
            windowBackground.w));

        if (ImGui::BeginChild("Viewport", ImVec2(availableAreaSize.x - m_SideBarWidth, availableAreaSize.y), false, windowFlags))
        {
            if (m_Sprite->GetTexture())
                RenderViewportContent();
        }
		ImGui::EndChild();

        ImGui::PopStyleColor();
    }

    void SpriteEditor::RenderViewportContent()
    {
        const Ref<Texture>& texture = m_Sprite->GetTexture();
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        ImVec2 windowScroll = window->Scroll;
        ImVec2 mousePosition = ImGui::GetMousePos() - window->Pos;

        ImVec2 textureSize = ImVec2((float)texture->GetWidth(), (float)texture->GetHeight());
        float textureAspectRatio = textureSize.x / textureSize.y;

        ImVec2 maxTextureSize = window->Size - window->ScrollbarSizes;
        ImVec2 scaledTextureSize = ImVec2(maxTextureSize.x, maxTextureSize.x / textureAspectRatio);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            ImVec2 viewportDragDelta = ImGui::GetIO().MouseDelta;
            ImGui::SetScrollX(window->Scroll.x - viewportDragDelta.x);
            ImGui::SetScrollY(window->Scroll.y - viewportDragDelta.y);
        }

        ImGui::Image((ImTextureID)texture->GetRendererId(), textureSize * m_Zoom, ImVec2(0, 1), ImVec2(1, 0));
        ImRect imageRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
        bool imageHovered = ImGui::IsMouseHoveringRect(imageRect.Min, imageRect.Max);

        ImVec2 mousePositionTextureSpace = WindowToTextureSpace(mousePosition);
        mousePositionTextureSpace.x = glm::round(mousePositionTextureSpace.x);
        mousePositionTextureSpace.y = glm::round(mousePositionTextureSpace.y);

        mousePositionTextureSpace.x = glm::clamp(mousePositionTextureSpace.x, 0.0f, textureSize.x);
        mousePositionTextureSpace.y = glm::clamp(mousePositionTextureSpace.y, 0.0f, textureSize.y);

        float scrollInput = ImGui::GetIO().MouseWheel;
        m_Zoom = glm::max(1.0f, m_Zoom + scrollInput);

        if (glm::abs(scrollInput) > 0.0f)
        {
            ImVec2 position = TextureToWindowSpace(mousePositionTextureSpace);
            ImVec2 newScroll = position - mousePosition;

            ImGui::SetScrollX(window->Scroll.x + newScroll.x);
            ImGui::SetScrollY(window->Scroll.y + newScroll.y);
        }

        SelectionRectSide selectionSides = RenderSelectionRect(imageRect);

        SelectionRectSide currentSelectionSides = selectionSides;
        if (m_ResizedSides != SelectionRectSide::None)
            currentSelectionSides = m_ResizedSides;

        if (currentSelectionSides == SelectionRectSide::TopLeft || currentSelectionSides == SelectionRectSide::BottomRight)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        else if (currentSelectionSides == SelectionRectSide::BottomLeft || currentSelectionSides == SelectionRectSide::TopRight)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        else if (HAS_BIT(currentSelectionSides, SelectionRectSide::Left) || HAS_BIT(currentSelectionSides, SelectionRectSide::Right))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        else if (HAS_BIT(currentSelectionSides, SelectionRectSide::Top) || HAS_BIT(currentSelectionSides, SelectionRectSide::Bottom))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        if (m_ResizedSides != SelectionRectSide::None)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (m_ResizedSides == SelectionRectSide::TopLeft)
                    m_SelectionStart = mousePositionTextureSpace;
                else if (m_ResizedSides == SelectionRectSide::BottomRight)
                    m_SelectionEnd = mousePositionTextureSpace;
                else if (m_ResizedSides == SelectionRectSide::TopRight)
                {
                    m_SelectionEnd.x = mousePositionTextureSpace.x;
                    m_SelectionStart.y = mousePositionTextureSpace.y;
                }
                else if (m_ResizedSides == SelectionRectSide::BottomLeft)
                {
                    m_SelectionStart.x = mousePositionTextureSpace.x;
                    m_SelectionEnd.y = mousePositionTextureSpace.y;
                }
                else if (m_ResizedSides == SelectionRectSide::Top)
                    m_SelectionStart.y = mousePositionTextureSpace.y;
                else if (m_ResizedSides == SelectionRectSide::Right)
                    m_SelectionEnd.x = mousePositionTextureSpace.x;
                else if (m_ResizedSides == SelectionRectSide::Bottom)
                    m_SelectionEnd.y = mousePositionTextureSpace.y;
                else if (m_ResizedSides == SelectionRectSide::Left)
                    m_SelectionStart.x = mousePositionTextureSpace.x;
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                m_ResizedSides = SelectionRectSide::None;
                ValidateSelectionRect();
            }
        }
        else
        {
            bool leftButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            if (leftButtonDown && imageHovered)
            {
                m_CanResize = true;
            }

            if (leftButtonDown && m_CanResize)
            {
                if (selectionSides == SelectionRectSide::None)
                {
                    if (!m_SelectionStarted)
                    {
                        m_SelectionStart = mousePositionTextureSpace;
                        m_SelectionStarted = true;
                        m_HasSelection = true;
                    }

                    m_SelectionEnd = mousePositionTextureSpace;
                }
                else
                {
                    m_ResizedSides = selectionSides;
                    ValidateSelectionRect();
                }
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                m_SelectionStarted = false;
                m_CanResize = false;
            }
        }
    }

    void SpriteEditor::RenderSidebar()
    {
        if (ImGui::BeginChild("Sidebar"))
            RenderSidebarContent();
		ImGui::EndChild();
    }

    void SpriteEditor::RenderSidebarContent()
    {
        const Ref<Texture>& texture = m_Sprite->GetTexture();

        const auto& style = ImGui::GetStyle();
        const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
        if (ImGui::Button("Save", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight)))
        {
            if (texture)
            {
                Math::Rect uvRect = SelectionToUVRect();

                m_Sprite->UVMin = uvRect.Min;
                m_Sprite->UVMax = uvRect.Max;
            }
            else
            {
                m_Sprite->UVMin = glm::vec2(0.0f);
                m_Sprite->UVMax = glm::vec2(1.0f);
            }

            if (AssetManager::IsAssetHandleValid(m_Sprite->Handle))
            {
                const AssetMetadata* metadata = AssetManager::GetAssetMetadata(m_Sprite->Handle);
                SpriteImporter::SerializeSprite(m_Sprite, metadata->Path);
            }
        }

        if (EditorGUI::BeginPropertyGrid())
        {
            AssetHandle textureHandle = texture ? texture->Handle : NULL_ASSET_HANDLE;
            if (EditorGUI::AssetField("Texture", textureHandle, &Texture::_Asset))
            {
                Ref<Texture> texture = AssetManager::GetAsset<Texture>(textureHandle);

                m_Sprite->SetTexture(texture);

                if (!texture)
                    ResetSelection();
            }

            glm::vec2 textureSize(0.0f);
            glm::ivec2 position(0);
            glm::ivec2 size(0);

            if (texture)
            {
                Math::Rect uvRect = SelectionToUVRect();
                textureSize = glm::vec2((float)texture->GetWidth(), (float)texture->GetHeight());

                position = (glm::ivec2)(uvRect.Min * textureSize);
                size = (glm::ivec2)(uvRect.GetSize() * textureSize);
            }

            ImGui::BeginDisabled(texture == nullptr);

            bool shouldClamp = false;
            if (EditorGUI::IntVector2PropertyField("Position", position))
            {
                ValidateSelectionRect();
                m_SelectionStart = ImVec2((float)position.x, (float)position.y);
                m_SelectionEnd = ImVec2((float)(position.x + size.x), (float)(position.y + size.y));

                shouldClamp = true;
            }

            if (EditorGUI::IntVector2PropertyField("Size", size))
            {
                ValidateSelectionRect();
                m_SelectionEnd = ImVec2((float)(position.x + size.x), (float)(position.y + size.y));
                shouldClamp = true;
            }

            if (shouldClamp)
            {
                m_SelectionStart.x = glm::clamp(m_SelectionStart.x, 0.0f, textureSize.x);
                m_SelectionStart.y = glm::clamp(m_SelectionStart.y, 0.0f, textureSize.y);

                m_SelectionEnd.x = glm::clamp(m_SelectionEnd.x, 0.0f, textureSize.x);
                m_SelectionEnd.y = glm::clamp(m_SelectionEnd.y, 0.0f, textureSize.y);
            }

            ImGui::EndDisabled();
            EditorGUI::EndPropertyGrid();
        }
    }

    void SpriteEditor::ResetSelection()
    {
        m_HasSelection = false;
        m_SelectionStarted = false;
        m_SelectionStart = ImVec2(0.0f, 0.0f);
        m_SelectionEnd = ImVec2(0.0f, 0.0f);
        m_ResizedSides = SelectionRectSide::None;
    }

    ImVec2 SpriteEditor::WindowToTextureSpace(ImVec2 windowSpace)
    {
        return ImVec2((windowSpace + ImGui::GetCurrentWindow()->Scroll) / m_Zoom);
    }

    ImVec2 SpriteEditor::TextureToWindowSpace(ImVec2 textureSpace)
    {
        return ImVec2(textureSpace * m_Zoom - ImGui::GetCurrentWindow()->Scroll);
    }

    SpriteEditor::SelectionRectSide SpriteEditor::RenderSelectionRect(ImRect imageRect)
    {
        if (!m_HasSelection)
            return SelectionRectSide::None;
        
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* drawList = window->DrawList;

        ImU32 defaultColor = 0xffffffff;
        ImU32 contrastColor = 0xff000000;

        ImRect selectionBounds = { TextureToWindowSpace(m_SelectionStart), TextureToWindowSpace(m_SelectionEnd) };

        if (selectionBounds.Max.x < selectionBounds.Min.x)
            std::swap(selectionBounds.Min.x, selectionBounds.Max.x);
        if (selectionBounds.Max.y < selectionBounds.Min.y)
            std::swap(selectionBounds.Min.y, selectionBounds.Max.y);

        ImRect selectionUIRectBounds = { selectionBounds.Min + imageRect.Min + window->Scroll, selectionBounds.Max + imageRect.Min + window->Scroll };

        if (selectionUIRectBounds.GetWidth() != 0 && selectionUIRectBounds.GetHeight() != 0)
        {
            // Inner rect
            drawList->AddRect(
                selectionUIRectBounds.Min + ImVec2(1.0f, 1.0f),
                selectionUIRectBounds.Max - ImVec2(1.0f, 1.0f),
                contrastColor);
        }

        // Middle rect
        drawList->AddRect(
            selectionUIRectBounds.Min,
            selectionUIRectBounds.Max,
            defaultColor);

        // Outer rect
        drawList->AddRect(
            selectionUIRectBounds.Min - ImVec2(1.0f, 1.0f),
            selectionUIRectBounds.Max + ImVec2(1.0f, 1.0f),
            contrastColor);

        ImVec2 cornerSize = ImVec2(m_SelectionRectCornerSize, m_SelectionRectCornerSize);
        SelectionRectSide result = SelectionRectSide::None;

        ImRect scrolledSelectionBounds = { selectionBounds.Min + window->Scroll, selectionBounds.Max + window->Scroll };

        float sideWidth = cornerSize.x + scrolledSelectionBounds.GetWidth();
        float sideHeight = cornerSize.y + scrolledSelectionBounds.GetHeight();

        ImVec2 topRightCorner = ImVec2(scrolledSelectionBounds.Max.x, scrolledSelectionBounds.Min.y);
        ImVec2 bottomLeftCorner = ImVec2(scrolledSelectionBounds.Min.x, scrolledSelectionBounds.Max.y);

        if (RenderSelectionSide("Top", scrolledSelectionBounds.Min - cornerSize / 2.0f, ImVec2(sideWidth, cornerSize.y)))
            result |= SelectionRectSide::Top;
        if (RenderSelectionSide("Left", scrolledSelectionBounds.Min - cornerSize / 2.0f, ImVec2(cornerSize.x, sideHeight)))
            result |= SelectionRectSide::Left;
        if (RenderSelectionSide("Right", topRightCorner - cornerSize / 2.0f, ImVec2(cornerSize.x, sideHeight)))
            result |= SelectionRectSide::Right;
        if (RenderSelectionSide("Bottom", bottomLeftCorner - cornerSize / 2.0f, ImVec2(sideWidth, cornerSize.y)))
            result |= SelectionRectSide::Bottom;

        ImVec2 corners[] =
        {
            selectionUIRectBounds.Min,
            ImVec2(selectionUIRectBounds.Max.x, selectionUIRectBounds.Min.y),
            selectionUIRectBounds.Max,
            ImVec2(selectionUIRectBounds.Min.x, selectionUIRectBounds.Max.y),
        };

        for (size_t i = 0; i < 4; i++)
        {
            drawList->AddRectFilled(corners[i] - cornerSize / 2.0f, corners[i] + cornerSize / 2.0f, 0xffffffff);
            drawList->AddRect(corners[i] - cornerSize / 2.0f, corners[i] + cornerSize / 2.0f, 0xff000000);
        }

        return !m_SelectionStarted ? result : SelectionRectSide::None;
    }

    bool SpriteEditor::RenderSelectionSide(const char* name, ImVec2 position, ImVec2 size)
    {
        ImGui::SetCursorPos(position);
        ImGui::InvisibleButton(name, size);

        return ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }

    void SpriteEditor::ValidateSelectionRect()
    {
        if (m_SelectionEnd.x < m_SelectionStart.x)
            std::swap(m_SelectionStart.x, m_SelectionEnd.x);
        if (m_SelectionEnd.y < m_SelectionStart.y)
            std::swap(m_SelectionStart.y, m_SelectionEnd.y);
    }

    Math::Rect SpriteEditor::SelectionToUVRect()
    {
        const Ref<Texture>& texture = m_Sprite->GetTexture();
        FLARE_CORE_ASSERT(texture);

        ImVec2 textureSize = ImVec2((float)texture->GetWidth(), (float)texture->GetHeight());
        glm::vec2 uvMin = glm::min(
            glm::vec2(m_SelectionStart.x, m_SelectionStart.y),
            glm::vec2(m_SelectionEnd.x, m_SelectionEnd.y)) / glm::vec2(textureSize.x, textureSize.y);

        glm::vec2 uvMax = glm::max(
            glm::vec2(m_SelectionStart.x, m_SelectionStart.y),
            glm::vec2(m_SelectionEnd.x, m_SelectionEnd.y)) / glm::vec2(textureSize.x, textureSize.y);

        // Flip vertically because ImGui's Y axis is opposite to texture coordinates Y
        uvMin.y = 1.0f - uvMin.y;
        uvMax.y = 1.0f - uvMax.y;
        std::swap(uvMin.y, uvMax.y);
        return Math::Rect(uvMin, uvMax);
    }

    void SpriteEditor::OnEvent(Event& event)
    {
    }

    void SpriteEditor::OnOpen(AssetHandle asset)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(asset));
        const AssetMetadata* metadata = AssetManager::GetAssetMetadata(asset);

        FLARE_CORE_ASSERT(metadata->Type == AssetType::Sprite);

        m_Sprite = AssetManager::GetAsset<Sprite>(asset);
        FLARE_CORE_ASSERT(m_Sprite);

        const Ref<Texture>& texture = m_Sprite->GetTexture();
        if (texture)
        {
            ImVec2 textureSize = ImVec2((float)texture->GetWidth(), (float)texture->GetHeight());
            m_SelectionStart = ImVec2(textureSize.x * m_Sprite->UVMin.x, textureSize.y * m_Sprite->UVMin.y);
            m_SelectionEnd = ImVec2(textureSize.x * m_Sprite->UVMax.x, textureSize.y * m_Sprite->UVMax.y);

            // Flip vertically
            m_SelectionStart.y = textureSize.y - m_SelectionStart.y;
            m_SelectionEnd.y = textureSize.y - m_SelectionEnd.y;
            
            m_HasSelection = true;
        }
    }

    void SpriteEditor::OnClose()
    {
        m_Sprite = nullptr;
        m_Zoom = 1.0f;
        m_CanResize = false;

        ResetSelection();
    }

    void SpriteEditor::OnRenderImGui(bool& show)
    {
        if (ImGui::Begin("Sprite Editor", &show))
            RenderWindowContent();

        ImGui::End();
    }
}
