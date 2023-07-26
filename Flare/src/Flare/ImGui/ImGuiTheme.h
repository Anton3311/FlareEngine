#pragma once

#include <stdint.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

namespace Flare
{
	ImVec4 ColorFromHex(uint32_t hex);
	
	struct ImGuiTheme
	{
		static ImVec4 Text;
		static ImVec4 TextDisabled;
		static ImVec4 TextSelectionBackground;

		static ImVec4 WindowBackground;
		static ImVec4 WindowBorder;

		static ImVec4 FrameBorder;
		static ImVec4 FrameBackground;
		static ImVec4 FrameHoveredBackground;
		static ImVec4 FrameActiveBackground;

		static ImVec4 Primary;
		static ImVec4 PrimaryVariant;

		static ImVec4 Surface;
	};
}