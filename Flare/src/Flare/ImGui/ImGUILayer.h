#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"
#include "Flare/ImGui/ImGuiTheme.h"

namespace Flare
{
	class ImGUILayer : public Layer
	{
	public:
		ImGUILayer();
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void Begin();
		void End();
	private:
		void SetThemeColors();
	};
}