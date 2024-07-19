#include "Sprite.h"

namespace Flare
{
	FLARE_IMPL_ASSET(Sprite);
	FLARE_SERIALIZABLE_IMPL(Sprite);

	Sprite::Sprite()
		: Asset(AssetType::Sprite), m_Texture(nullptr), UVMin(0.0f), UVMax(0.0f)
	{
	}

	Sprite::Sprite(const Ref<Texture>& texture, glm::i32vec2 pixelsMin, glm::i32vec2 pixelsMax)
		: Asset(AssetType::Sprite), m_Texture(texture)
	{
		glm::vec2 size = glm::vec2((float)texture->GetWidth(), (float)texture->GetHeight());
		UVMin = glm::vec2(pixelsMin) / size;
		UVMax = glm::vec2(pixelsMax) / size;
	}
}