#pragma once

#include "FlarePlatform/WindowControls.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace Flare
{
	class WindowsWindowControls : public WindowControls
	{
	public:
		bool BeginTitleBar();
		void EndTitleBar();

		void RenderControls();

		virtual bool IsTitleBarHovered() const override { return m_TitleBarHovered; }
	private:
		bool RenderControlsButton(const char* name, ImU32& iconColor);
	private:
		float m_TitleBarHeight = 40.0f;
		bool m_TitleBarHovered = false;
	};
}