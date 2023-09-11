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
		glm::vec3 Position;

		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 ViewProjection = glm::mat4(1.0f);

		glm::mat4 InverseProjection = glm::mat4(1.0f);
		glm::mat4 InverseView = glm::mat4(1.0f);
		glm::mat4 InverseViewProjection = glm::mat4(1.0f);

		void CalculateViewProjection()
		{
			ViewProjection = Projection * View;
			InverseView = glm::inverse(View);
			InverseViewProjection = glm::inverse(ViewProjection);
		}
	};

	struct RenderData
	{
		CameraData Camera;
		ViewportRect Viewport;
		bool IsEditorCamera = false;
	};
}