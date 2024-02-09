#include "SpriteEditor.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>

namespace Flare
{
    FLARE_IMPL_ENUM_BITFIELD(SpriteEditor::SelectionRectSide);

    SpriteEditor::SpriteEditor(const Ref<Sprite>& sprite)
        : m_Sprite(sprite)
    {
        const Ref<Texture>& texture = sprite->GetTexture();
        if (texture)
        {
            ImVec2 textureSize = ImVec2((float)texture->GetWidth(), (float)texture->GetHeight());
            m_SelectionStart = ImVec2(textureSize.x * sprite->UVMin.x, textureSize.y * sprite->UVMin.y);
            m_SelectionEnd = ImVec2(textureSize.x * sprite->UVMax.x, textureSize.y * sprite->UVMax.y);
        }
    }

    bool SpriteEditor::OnImGuiRenderer()
    {
        bool result = false;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysHorizontalScrollbar
            | ImGuiWindowFlags_AlwaysVerticalScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoMove;

        if (!ImGui::BeginChild("Viewport", ImVec2(0, 0), false, windowFlags))
        {
            ImGui::EndChild();
            return false;
        }

        const Ref<Texture>& texture = m_Sprite->GetTexture();
        if (!texture)
        {
            ImGui::EndChild();
            return false;
        }

        ImGuiWindow* window = ImGui::GetCurrentWindow();

        ImVec2 windowScroll = window->Scroll;
        ImVec2 mousePosition = ImGui::GetMousePos() - window->Pos;

        ImVec2 textureSize = ImVec2((float)texture->GetWidth(), (float)texture->GetHeight());
        float textureAspectRatio = textureSize.x / textureSize.y;

        ImVec2 maxTextureSize = window->Size - window->ScrollbarSizes;
        ImVec2 scaledTextureSize = ImVec2(maxTextureSize.x, maxTextureSize.x / textureAspectRatio);

        float scrollInput = ImGui::GetIO().MouseWheel;
        m_Zoom = glm::max(1.0f, m_Zoom + scrollInput);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            ImVec2 viewportDragDelta = ImGui::GetIO().MouseDelta;
            ImGui::SetScrollX(window->Scroll.x - viewportDragDelta.x);
            ImGui::SetScrollY(window->Scroll.y - viewportDragDelta.y);
        }

        ImGui::Image((ImTextureID)texture->GetRendererId(), textureSize * m_Zoom, ImVec2(0, 1), ImVec2(1, 0));
        bool imageHovered = ImGui::IsItemHovered();

        ImRect imageRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

        ImVec2 mousePositionTextureSpace = WindowToTextureSpace(mousePosition);
        mousePositionTextureSpace.x = glm::round(mousePositionTextureSpace.x);
        mousePositionTextureSpace.y = glm::round(mousePositionTextureSpace.y);

        mousePositionTextureSpace.x = glm::clamp(mousePositionTextureSpace.x, 0.0f, textureSize.x);
        mousePositionTextureSpace.y = glm::clamp(mousePositionTextureSpace.y, 0.0f, textureSize.y);

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
        else if (imageHovered)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (selectionSides == SelectionRectSide::None)
                {
                    if (!m_SelectionStarted)
                    {
                        m_SelectionStart = mousePositionTextureSpace;
                        m_SelectionStarted = true;
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
            }
        }

        glm::vec2 uvMin = glm::min(glm::vec2(m_SelectionStart.x, m_SelectionStart.y), glm::vec2(m_SelectionEnd.x, m_SelectionEnd.y)) / glm::vec2(textureSize.x, textureSize.y);
        glm::vec2 uvMax = glm::max(glm::vec2(m_SelectionStart.x, m_SelectionStart.y), glm::vec2(m_SelectionEnd.x, m_SelectionEnd.y)) / glm::vec2(textureSize.x, textureSize.y);

        m_Sprite->UVMin = uvMin;
        m_Sprite->UVMax = uvMax;

        ImGui::EndChild();
        return result;
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
}
