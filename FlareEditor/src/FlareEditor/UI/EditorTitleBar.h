#pragma once

#include "FlareCore/Core.h"

#include "FlareEditor/UI/WindowsWindowControls.h"

#include <glm/glm.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace Flare
{
	class EditorLayer;
	class EditorTitleBar
	{
	public:
		EditorTitleBar();

		void OnRenderImGui();
	private:
		void RenderTitleBar();
		bool RenderButton(const char* id, glm::ivec2 iconPosition);
	private:
		Ref<WindowsWindowControls> m_WindowControls;
	};
}