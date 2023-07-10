#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"

#include "FlareEditor/UI/SceneWindow.h"
#include "FlareEditor/UI/PropertiesWindow.h"
#include "FlareEditor/UI/AssetManagerWindow.h"

#include "FlareEditor/ViewportWindow.h"

#include <vector>

namespace Flare
{
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
	public:
		virtual void OnAttach() override;
		virtual void OnUpdate(float deltaTime) override;

		virtual void OnEvent(Event& event) override;
		virtual void OnImGUIRender() override;
	private:
		float m_PreviousFrameTime = 0.0f;

		SceneWindow m_SceneWindow;
		PropertiesWindow m_PropertiesWindow;
		AssetManagerWindow m_AssetManagerWindow;

		std::vector<ViewportWindow> m_Viewports;
	};
}