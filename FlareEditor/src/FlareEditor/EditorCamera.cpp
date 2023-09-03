#include "EditorCamera.h"

#include "FlareCore/Log.h"
#include "FlarePlatform/Events.h"

#include "Flare/Renderer2D/Renderer2D.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Flare
{
	EditorCamera::EditorCamera()
	{
		RecalculateViewMatrix();
	}

	void EditorCamera::ProcessEvents(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) -> bool
		{
			m_IsControlled = event.GetMouseCode() == MouseCode::ButtonMiddle;
			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& event) -> bool
		{
			if (m_IsControlled && event.GetMouseCode() == MouseCode::ButtonMiddle)
				m_IsControlled = false;
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
			glm::vec2 delta = event.GetPosition() - m_PreviousMousePosition;
			m_PreviousMousePosition = event.GetPosition();

			if (m_IsControlled && !m_IsMoved)
				Rotate(delta);
			else if (m_IsControlled && m_IsMoved)
				Drag(delta);

			return false;
		});

		dispatcher.Dispatch<MouseScrollEvent>([this](MouseScrollEvent& event) -> bool
		{
			glm::vec2 scroll = event.GetOffset();
			Zoom(-scroll.y);
			return false;
		});
	}

	void EditorCamera::OnViewportChanged(const ViewportRect& viewport)
	{
		m_Viewport = viewport;

		float aspectRatio = (float)viewport.Size.x / (float)viewport.Size.y;
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

	void EditorCamera::Drag(glm::vec2 mouseInput)
	{
		mouseInput *= m_Settings.DragSpeed;

		glm::vec3 right = TransformDirection(glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 up = TransformDirection(glm::vec3(0.0f, 1.0f, 0.0f));

		glm::vec3 movementDirection = -right * mouseInput.x + up * mouseInput.y;
		m_Origin += movementDirection;

		RecalculateViewMatrix();
	}

	void EditorCamera::RecalculateViewMatrix()
	{
		glm::vec3 position = m_Origin - TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)) * m_DistanceToOrigin;
		m_ViewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(glm::radians(-m_Rotation))));
	}

	glm::vec3 EditorCamera::TransformDirection(const glm::vec3& direction)
	{
		return glm::rotate(glm::quat(glm::radians(-m_Rotation)), direction);
	}
}
