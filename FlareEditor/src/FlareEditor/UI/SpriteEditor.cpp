#include "SpriteEditor.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>

namespace Flare
{
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

        ImVec2 textureSize = ImVec2(texture->GetWidth(), texture->GetHeight());
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

        ImRect imageRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

        ImVec2 mousePositionTextureSpace = WindowToTextureSpace(mousePosition);
        mousePositionTextureSpace.x = glm::round(mousePositionTextureSpace.x);
        mousePositionTextureSpace.y = glm::round(mousePositionTextureSpace.y);

        mousePositionTextureSpace.x = glm::clamp(mousePositionTextureSpace.x, 0.0f, textureSize.x);
        mousePositionTextureSpace.y = glm::clamp(mousePositionTextureSpace.y, 0.0f, textureSize.y);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
			if (!m_SelectionStarted)
			{
				m_SelectionStart = mousePositionTextureSpace;
				m_SelectionStarted = true;
			}

            m_SelectionEnd = mousePositionTextureSpace;
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_SelectionStarted = false;
        }

        ImDrawList* drawList = window->DrawList;
        drawList->AddRect(
            imageRect.Min + TextureToWindowSpace(m_SelectionStart) + window->Scroll,
            imageRect.Min + TextureToWindowSpace(m_SelectionEnd) + window->Scroll,
            0xffffffff);

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
}
