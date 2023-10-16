#pragma once

#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"

#include "FlarePlatform/Event.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Flare
{
	struct EditorCameraSettings
	{
		float FOV = 0.1f;
		float Near = 1000.0f;
		float Far = 60.0f;

		float RotationSpeed = 1.0f;
	};

	class EditorCamera
	{
	public:
		EditorCamera();

		EditorCameraSettings& GetSettings() { return m_Settings; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

		glm::vec3 GetPosition() const;
		inline glm::vec3 GetRotationOrigin() const { return m_Origin; }
		inline float GetZoom() const { return m_DistanceToOrigin; }

		void ProcessEvents(Event& event);
		void OnViewportChanged(glm::ivec2 viewportSize, glm::ivec2 viewportPosition);

		void Zoom(float amount);
		void Rotate(glm::vec2 mouseInput);
		void Drag(glm::vec2 mouseInput);
	private:
		void RecalculateViewMatrix();
		glm::vec3 CalculateTranslationPoint(glm::vec2 mousePosition, const glm::mat4& inverseProjection, const glm::mat4& inverseView);

		glm::vec3 TransformDirection(const glm::vec3& direction) const;
	private:
		EditorCameraSettings m_Settings;
		glm::vec2 m_PreviousMousePosition = glm::vec2(0.0f);

		bool m_IsMoved = false;
		bool m_IsControlled = false;

		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);

		glm::vec3 m_Origin = glm::vec3(0.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f);

		glm::vec2 m_ViewportPosition;
		glm::vec2 m_ViewportSize;

		float m_DistanceToOrigin = 10.0f;
	};
}