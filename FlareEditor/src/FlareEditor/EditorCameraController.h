#pragma once

#include "FlareEditor/EditorCamera.h"

namespace Flare
{
	class EditorCameraController
	{
	public:
		EditorCameraController(EditorCamera& camera)
			: m_Camera(camera) {}

		void OnWindowFocus();
		void Update(glm::vec2 mousePosition);
	private:
		EditorCamera& m_Camera;

		bool m_IsMouseDown = false;
	};
}
