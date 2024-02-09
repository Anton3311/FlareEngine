#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Sprite.h"

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
    private:
        Ref<Sprite> m_Sprite;

        float m_Zoom = 1.0f;
    };
}
