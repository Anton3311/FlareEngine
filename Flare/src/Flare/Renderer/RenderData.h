#pragma once

#include "Flare/Renderer/CameraData.h"

namespace Flare
{
	struct RenderData
	{
		CameraData Camera;
		glm::u32vec2 ViewportSize = glm::u32vec2(0);
		bool IsEditorCamera = false;
	};
}