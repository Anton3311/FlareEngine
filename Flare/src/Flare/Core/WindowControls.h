#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace Flare
{
	class WindowControls
	{
	public:
		void BeginTitleBar();
		void EndTitleBar();

		void RenderControls();

		bool IsTitleBatHovered() const { return m_TitleBarHovered; }
	private:
		bool RenderControlsButton(const char* name, ImU32& iconColor);
	private:
		float m_TitleBarHeight = 40.0f;
		bool m_TitleBarHovered = false;
	};
}