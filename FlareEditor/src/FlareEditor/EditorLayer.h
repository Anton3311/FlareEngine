#pragma once

#include "Flare.h"
#include "Flare/Core/Layer.h"

#include "FlareECS/World.h"
#include "FlareECS/Registry.h"

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
		void CalculateProjection(float size);
		void DrawComponent(Entity entity, ComponentId componentId);
	private:
		Ref<Shader> m_QuadShader;
		Ref<FrameBuffer> m_FrameBuffer;

		glm::mat4 m_ProjectionMatrix = glm::mat4(0.0f);
		glm::mat4 m_InverseProjection = glm::mat4(0.0f);

		float m_CameraSize = 24.0f;
		float m_PreviousFrameTime = 0.0f;

		int32_t m_Width = 20;
		int32_t m_Height = 20;

		glm::i32vec2 m_ViewportSize = glm::i32vec2(0.0f);

		World m_World;
		QueryId m_Query;
		Entity m_TestEntity;
	};
}