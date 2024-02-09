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
            | ImGuiWindowFlags_NoScrollWithMouse;

        if (!ImGui::BeginChild("Viewport", ImVec2(0, 0), false, windowFlags))
        {
            ImGui::EndChild();
            return false;
        }

        const Ref<Texture>& texture = m_Sprite->GetTexture();
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

        ImGui::EndChild();
        return result;
    }
}
