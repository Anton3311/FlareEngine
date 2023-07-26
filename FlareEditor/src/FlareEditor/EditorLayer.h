#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"

#include "FlareEditor/UI/SceneWindow.h"
#include "FlareEditor/UI/PropertiesWindow.h"
#include "FlareEditor/UI/AssetManagerWindow.h"

#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"

#include <vector>

namespace Flare
{
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float deltaTime) override;

		virtual void OnEvent(Event& event) override;
		virtual void OnImGUIRender() override;

		void SaveActiveScene();
		void SaveActiveSceneAs();

		void EnterPlayMode();
		void ExitPlayMode();

		static EditorLayer& GetInstance();
	private:
		void UpdateWindowTitle();
	private:
		float m_PreviousFrameTime = 0.0f;

		SceneWindow m_SceneWindow;
		PropertiesWindow m_PropertiesWindow;
		AssetManagerWindow m_AssetManagerWindow;

		std::vector<Ref<ViewportWindow>> m_Viewports;
		glm::vec4 m_ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		EditorCamera m_Camera;

		static EditorLayer* s_Instance;
	};
}