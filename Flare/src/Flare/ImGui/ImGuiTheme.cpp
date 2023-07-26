#include "ImGuiTheme.h"

namespace Flare
{
	ImVec4 ColorFromHex(uint32_t hex)
	{
		uint8_t r = (uint8_t)((hex & 0xff000000) >> 24);
		uint8_t g = (uint8_t)((hex & 0x00ff0000) >> 16);
		uint8_t b = (uint8_t)((hex & 0x0000ff00) >> 8);
		uint8_t a = (uint8_t)((hex & 0x000000ff) >> 0);

		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}

	ImVec4 ImGuiTheme::Text = ColorFromHex(0xffffffff);
	ImVec4 ImGuiTheme::TextDisabled = ColorFromHex(0x8A8F98ff);
	ImVec4 ImGuiTheme::TextSelectionBackground = ColorFromHex(0x4E9F3Dff); // Same as primary

	ImVec4 ImGuiTheme::WindowBackground = ColorFromHex(0x191A19ff);
	ImVec4 ImGuiTheme::WindowBorder = ColorFromHex(0x505250ff);

	ImVec4 ImGuiTheme::FrameBorder = ColorFromHex(0x505250ff);
	ImVec4 ImGuiTheme::FrameBackground = ColorFromHex(0x383938ff);
	ImVec4 ImGuiTheme::FrameHoveredBackground = ColorFromHex(0x585958ff);
	ImVec4 ImGuiTheme::FrameActiveBackground = ColorFromHex(0x4C4E4Cff);

	ImVec4 ImGuiTheme::Primary = ColorFromHex(0x4E9F3Dff);
	ImVec4 ImGuiTheme::PrimaryVariant = ColorFromHex(0x256917ff);

	ImVec4 ImGuiTheme::Surface = ColorFromHex(0x383938ff);
}
