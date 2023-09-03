#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderData.h"

#include <glm/glm.hpp>

namespace Flare
{
	class FLARE_API Renderer
	{
	public:
		static void SetMainViewportSize(glm::uvec2 size);
		static glm::uvec2 GetMainViewportSize();
	};
}