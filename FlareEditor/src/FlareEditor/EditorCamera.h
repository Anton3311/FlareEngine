#pragma once

#include <glm/glm.hpp>

namespace Flare
{
	struct EditorCameraSettings
	{
		float FOV = 60.0f;
		float Near = 0.1f;
		float Far = 1000.0f;

		float RotationSpeed = 1.0f;
	};

	struct RenderView;
	class EditorCamera
	{
	public:
		EditorCamera();

		EditorCameraSettings& GetSettings() { return m_Settings; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

		glm::vec3 GetPosition() const;
		inline glm::vec3 GetRotation() const { return m_Rotation; }
		inline glm::vec3 GetRotationOrigin() const { return m_Origin; }
		inline float GetZoom() const { return m_DistanceToOrigin; }

		void FillRenderView(RenderView& view);

		void SetRotationOrigin(glm::vec3 position);
		void SetRotation(glm::vec3 rotation);
		void SetZoom(float zoom);

		void OnViewportChanged(glm::ivec2 viewportSize, glm::ivec2 viewportPosition);

		void Zoom(float amount);
		void Rotate(glm::vec2 mouseDelta);
		void Drag(glm::vec2 mousePosition);

		inline void ResetPreviousMousePosition(glm::vec2 newPosition) { m_PreviousMousePosition = newPosition; }

		inline glm::vec3 GetViewDirection() const { return TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)); }
	public:
		static constexpr float MinZoom = 0.1f;
	private:
		void RecalculateViewMatrix();
		glm::vec3 CalculateTranslationPoint(glm::vec2 mousePosition, const glm::mat4& inverseProjection, const glm::mat4& inverseView);

		glm::vec3 TransformDirection(const glm::vec3& direction) const;
	private:
		EditorCameraSettings m_Settings;
		glm::vec2 m_PreviousMousePosition = glm::vec2(0.0f);

		bool m_IsMoved = false;
		bool m_PreviousIsControlled = false;
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