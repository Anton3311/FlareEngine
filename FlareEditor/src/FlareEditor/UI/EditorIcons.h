#pragma once

#include "Flare/Renderer/Texture.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>

namespace Flare
{
	class EditorIcons
	{
	public:
		static constexpr glm::ivec2 TransformIcon = glm::ivec2(0, 0);
		static constexpr glm::ivec2 ObjectIcon = glm::ivec2(1, 0);
		static constexpr glm::ivec2 PlayIcon = glm::ivec2(2, 0);
		static constexpr glm::ivec2 PauseIcon = glm::ivec2(3, 0);
		static constexpr glm::ivec2 StopIcon = glm::ivec2(4, 0);
		static constexpr glm::ivec2 ContinueIcon = glm::ivec2(0, 1);
		static constexpr glm::ivec2 PointLightIcon = glm::ivec2(1, 1);
		static constexpr glm::ivec2 SpotlightIcon = glm::ivec2(2, 1);
	public:
		EditorIcons(int32_t iconSize);

		void Initialize();

		inline const Ref<Texture>& GetTexture() const { return m_IconsTexture; }
		inline int32_t GetIconSize() const { return m_IconSize; }

		ImRect GetIconUVs(glm::ivec2 position) const;
	private:
		int32_t m_IconSize;
		Ref<Texture> m_IconsTexture;
	};
}
