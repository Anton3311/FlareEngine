#pragma once

#include "Flare/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Flare
{
	class Sprite
	{
	public:
		Sprite() = default;
		Sprite(const Ref<Texture>& atlas, glm::i32vec2 pixelsMin, glm::i32vec2 pixelsMax);

		glm::vec2 GetUVMin() const { return m_UVMin; }
		glm::vec2 GetUVMax() const { return m_UVMax; }

		Ref<Texture> GetAtlas() const { return m_Atlas; }
	private:
		Ref<Texture> m_Atlas;
		glm::vec2 m_UVMin;
		glm::vec2 m_UVMax;
	};
}