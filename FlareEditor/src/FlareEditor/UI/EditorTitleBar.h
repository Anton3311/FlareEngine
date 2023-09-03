#pragma once

#include "FlareCore/Core.h"

#include "FlareEditor/UI/WindowsWindowControls.h"

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
	private:
		Ref<WindowsWindowControls> m_WindowControls;
	};
}