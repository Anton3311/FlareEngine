#include "EditorIcons.h"

namespace Flare
{
	EditorIcons::EditorIcons(int32_t iconSize)
		: m_IconsTexture(nullptr), m_IconSize(iconSize) {}

	void EditorIcons::Initialize()
	{
		TextureSpecifications specifications;
		specifications.Filtering = TextureFiltering::Linear;
		specifications.GenerateMipMaps = false;
		specifications.Wrap = TextureWrap::Clamp;

		m_IconsTexture = Texture::Create("assets/Icons.png", specifications);
	}

	ImRect EditorIcons::GetIconUVs(glm::ivec2 position) const
	{
		glm::vec2 textureSize = glm::vec2((float)m_IconsTexture->GetWidth(), (float)m_IconsTexture->GetHeight());

		glm::vec2 min = (glm::vec2)(position * m_IconSize) / textureSize;
		glm::vec2 max = (glm::vec2)((position + glm::ivec2(1, 1)) * m_IconSize) / textureSize;

		return ImRect{ ImVec2(min.x, 1.0f - min.y), ImVec2(max.x, 1.0f - max.y) };
	}
}
