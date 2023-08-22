#pragma once

#include <glm/glm.hpp>

namespace Flare
{
	struct ViewportRect
	{
		glm::ivec2 Position = glm::ivec2(0);
		glm::ivec2 Size = glm::ivec2(0);
	};

	struct CameraData
	{
		glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 ViewMatrix = glm::mat4(1.0f);

		glm::mat4 ViewProjectionMatrix = glm::mat4(1.0f);

		void CalculateViewProjection()
		{
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		}
	};

	struct RenderData
	{
		CameraData Camera;
		ViewportRect Viewport;
		bool IsEditorCamera = false;
	};
}