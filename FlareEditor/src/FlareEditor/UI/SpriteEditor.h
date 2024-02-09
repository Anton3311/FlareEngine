#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Sprite.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
    class SpriteEditor
    {
    public:
        enum class SelectionRectSide
        {
            None = 0,
            Top = 1,
            Right = 2,
            Bottom = 4,
            Left = 8,

            TopLeft = Top | Left,
            TopRight = Top | Right,
            BottomLeft = Bottom | Left,
            BottomRight = Bottom | Right,
        };

        SpriteEditor()
            : m_Sprite(nullptr) {}

        SpriteEditor(const Ref<Sprite>& sprite);

        bool OnImGuiRenderer();
    private:
        ImVec2 WindowToTextureSpace(ImVec2 windowSpace);
        ImVec2 TextureToWindowSpace(ImVec2 textureSpace);

        SelectionRectSide RenderSelectionRect(ImRect imageRect);
        bool RenderSelectionSide(const char* name, ImVec2 position, ImVec2 size);

        void ValidateSelectionRect();
    private:
        Ref<Sprite> m_Sprite = nullptr;
        SelectionRectSide m_ResizedSides = SelectionRectSide::None;

        float m_Zoom = 1.0f;
        float m_SelectionRectCornerSize = 8.0f;

        bool m_HasSelection = false;
        bool m_SelectionStarted = false;

        ImVec2 m_SelectionStart = ImVec2(0.0f, 0.0f);
        ImVec2 m_SelectionEnd = ImVec2(0.0f, 0.0f);
    };
}
