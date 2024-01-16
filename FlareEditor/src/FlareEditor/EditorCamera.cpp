#include "EditorCamera.h"

#include "FlareCore/Log.h"
#include "Flare/Math/Math.h"
#include "Flare/Input/InputManager.h"

#include "FlarePlatform/Events.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Flare
{
	EditorCamera::EditorCamera()
	{
		RecalculateViewMatrix();
	}

	glm::vec3 EditorCamera::GetPosition() const
	{
		return m_Origin - TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)) * m_DistanceToOrigin;
	}

	void EditorCamera::SetPosition(glm::vec3 position)
	{
		m_Origin = position;
		RecalculateViewMatrix();
	}

	void EditorCamera::ProcessEvents(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) -> bool
		{
			m_PreviousIsControlled = m_IsControlled;
			m_IsControlled = event.GetMouseCode() == MouseCode::ButtonMiddle;
			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& event) -> bool
		{
			if (m_IsControlled && event.GetMouseCode() == MouseCode::ButtonMiddle)
			{
				m_PreviousIsControlled = m_IsControlled;
				m_IsControlled = false;
			}

			return false;
		});

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) -> bool
		{
			m_IsMoved = event.GetKeyCode() == KeyCode::LeftShift;
			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& event) -> bool
		{
			if (m_IsMoved && event.GetKeyCode() == KeyCode::LeftShift)
				m_IsMoved = false;
			return false;
		});

		dispatcher.Dispatch<MouseMoveEvent>([this](MouseMoveEvent& event) -> bool
		{
			glm::vec2 delta = event.GetPosition() - m_ViewportPosition - m_PreviousMousePosition;

			if (!m_PreviousIsControlled && m_IsControlled)
			{
				delta = glm::vec2(0.0f);
				m_PreviousIsControlled = m_IsControlled;
			}

			if (m_IsControlled && !m_IsMoved)
				Rotate(delta);
			else if (m_IsControlled && m_IsMoved)
				Drag(event.GetPosition() - m_ViewportPosition);

			m_PreviousMousePosition = event.GetPosition() - m_ViewportPosition;

			return false;
		});

		dispatcher.Dispatch<MouseScrollEvent>([this](MouseScrollEvent& event) -> bool
		{
			glm::vec2 scroll = event.GetOffset();
			Zoom(-scroll.y);
			return false;
		});
	}

	void EditorCamera::OnViewportChanged(glm::ivec2 viewportSize, glm::ivec2 viewportPosition)
	{
		float aspectRatio = (float)viewportSize.x / (float)viewportSize.y;

		m_ViewportPosition = viewportPosition;
		m_ViewportSize = (glm::vec2)viewportSize;
		m_ProjectionMatrix = glm::perspective<float>(glm::radians(m_Settings.FOV), aspectRatio, m_Settings.Near, m_Settings.Far);
	}

	void EditorCamera::Zoom(float amount)
	{
		m_DistanceToOrigin = glm::max(m_DistanceToOrigin + amount, 0.1f);
		RecalculateViewMatrix();
	}

	void EditorCamera::Rotate(glm::vec2 mouseInput)
	{
		mouseInput *= m_Settings.RotationSpeed;

		m_Rotation.x += mouseInput.y;
		m_Rotation.y += mouseInput.x;

		RecalculateViewMatrix();
	}

	glm::vec3 EditorCamera::CalculateTranslationPoint(glm::vec2 mousePosition, const glm::mat4& inverseProjection, const glm::mat4& inverseView)
	{
		mousePosition = mousePosition / m_ViewportSize;
		glm::vec2 clipSpaceMousePosition = glm::vec2(mousePosition.x, 1.0f - mousePosition.y) * 2.0f - glm::vec2(1.0f);

		glm::vec3 direction = inverseProjection * glm::vec4(clipSpaceMousePosition, -1.0f, 1.0f);
		direction = inverseView * glm::vec4(direction, 0.0f);

		glm::vec3 cameraPosition = GetPosition();

		Math::Plane translationPlane = Math::Plane::TroughPoint(m_Origin, TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)));

		Math::Ray ray;
		ray.Direction = direction;
		ray.Origin = cameraPosition;

		float t = Math::IntersectPlane(translationPlane, ray);
		return cameraPosition + direction * t;
	}

	void EditorCamera::Drag(glm::vec2 mouseInput)
	{
		glm::mat4 inverseProjection = glm::inverse(m_ProjectionMatrix);
		glm::mat4 inverseView = glm::inverse(m_ViewMatrix);

		glm::vec3 previousPosition = CalculateTranslationPoint(mouseInput, inverseProjection, inverseView);
		glm::vec3 currentPosition = CalculateTranslationPoint(m_PreviousMousePosition, inverseProjection, inverseView);

		glm::vec3 movementDirection = (currentPosition - previousPosition);
		m_Origin += movementDirection;

		RecalculateViewMatrix();
	}

	void EditorCamera::RecalculateViewMatrix()
	{
		glm::vec3 position = GetPosition();
		m_ViewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(glm::radians(-m_Rotation))));
	}

	glm::vec3 EditorCamera::TransformDirection(const glm::vec3& direction) const
	{
		return glm::rotate(glm::quat(glm::radians(-m_Rotation)), direction);
	}
}
