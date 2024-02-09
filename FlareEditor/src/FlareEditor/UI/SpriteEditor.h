#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Sprite.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
    class SpriteEditor
    {
    public:
        SpriteEditor()
            : m_Sprite(nullptr) {}

        SpriteEditor(const Ref<Sprite>& sprite)
            : m_Sprite(sprite) {}

        bool OnImGuiRenderer();
        ImVec2 WindowToTextureSpace(ImVec2 windowSpace);
        ImVec2 TextureToWindowSpace(ImVec2 textureSpace);
    private:
        Ref<Sprite> m_Sprite;

        float m_Zoom = 1.0f;
        bool m_SelectionStarted = false;

        ImVec2 m_SelectionStart = ImVec2(0.0f, 0.0f);
        ImVec2 m_SelectionEnd = ImVec2(0.0f, 0.0f);
    };
}
