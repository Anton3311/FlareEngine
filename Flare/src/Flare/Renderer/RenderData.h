#pragma once

namespace Flare
{
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
		glm::u32vec2 ViewportSize = glm::uvec2(0);
		bool IsEditorCamera = false;
	};
}