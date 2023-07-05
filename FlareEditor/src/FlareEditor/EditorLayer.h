#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"

#include "FlareEditor/UI/SceneWindow.h"
#include "FlareEditor/UI/PropertiesWindow.h"

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
		Ref<FrameBuffer> m_FrameBuffer;

		float m_PreviousFrameTime = 0.0f;

		glm::i32vec2 m_ViewportSize = glm::i32vec2(0.0f);

		SceneWindow m_SceneWindow;
		PropertiesWindow m_PropertiesWindow;
	};
}