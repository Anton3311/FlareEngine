#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace Flare
{
	class EditorTitleBar
	{
	public:
		void OnRenderImGui();
	private:
		void RenderTitleBar();
	};
}