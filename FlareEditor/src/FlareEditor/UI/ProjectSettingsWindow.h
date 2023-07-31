#pragma once

namespace Flare
{
	class ProjectSettingsWindow
	{
	public:
		static void OnRenderImGui();

		static void Show();
	private:
		static bool m_Opened;
	};
}