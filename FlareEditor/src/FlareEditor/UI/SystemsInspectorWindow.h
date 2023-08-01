#pragma once

#include <stdint.h>

namespace Flare
{
	class SystemsInspectorWindow
	{
	public:
		static void OnImGuiRender();
		static void Show();
	private:
		static void RenderSystemItem(uint32_t systemIndex);
	private:
		static bool s_Opened;
		static uint32_t s_CurrentSystem;
	};
}