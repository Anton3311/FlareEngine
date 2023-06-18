#include "Sprite.h"

namespace Flare
{
	Sprite::Sprite(const Ref<Texture>& atlas, glm::i32vec2 pixelsMin, glm::i32vec2 pixelsMax)
		: m_Atlas(atlas)
	{
		glm::vec2 size = glm::vec2((float)atlas->GetWidth(), (float)atlas->GetHeight());
		m_UVMin = glm::vec2(pixelsMin) / size;
		m_UVMax = glm::vec2(pixelsMax) / size;
	}
}