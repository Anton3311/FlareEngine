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

	void EditorCamera::FillRenderView(RenderView& view)
	{
		view.SetViewAndProjection(GetProjectionMatrix(), GetViewMatrix());
		view.Position = GetPosition();
		view.ViewDirection = GetViewDirection();
		view.Near = GetSettings().Near;
		view.Far = GetSettings().Far;
		view.FOV = GetSettings().FOV;
	}

	void EditorCamera::SetRotationOrigin(glm::vec3 position)
	{
		m_Origin = position;
		RecalculateViewMatrix();
	}

	void EditorCamera::SetRotation(glm::vec3 rotation)
	{
		m_Rotation = rotation;
		RecalculateViewMatrix();
	}

	void EditorCamera::SetZoom(float zoom)
	{
		m_DistanceToOrigin = glm::max(zoom, MinZoom);
		RecalculateViewMatrix();
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
		m_DistanceToOrigin = glm::max(m_DistanceToOrigin + amount, MinZoom);
		RecalculateViewMatrix();
	}

	void EditorCamera::Rotate(glm::vec2 mouseDelta)
	{
		mouseDelta *= m_Settings.RotationSpeed;

		m_Rotation.x += mouseDelta.y;
		m_Rotation.y += mouseDelta.x;

		RecalculateViewMatrix();
	}

	glm::vec3 EditorCamera::CalculateTranslationPoint(glm::vec2 mousePosition, const glm::mat4& inverseProjection, const glm::mat4& inverseView)
	{
		glm::vec3 cameraPosition = GetPosition();

		mousePosition = mousePosition / m_ViewportSize;
		glm::vec2 clipSpaceMousePosition = glm::vec2(mousePosition.x, mousePosition.y) * 2.0f - glm::vec2(1.0f);

		glm::mat4 inverseViewProjection = glm::inverse(m_ProjectionMatrix * m_ViewMatrix);

		glm::vec4 relativeWorldSpacePosition = inverseViewProjection * glm::vec4(clipSpaceMousePosition, 1.0f, 1.0f);
		relativeWorldSpacePosition /= relativeWorldSpacePosition.w;

		glm::vec3 direction = glm::normalize((glm::vec3)relativeWorldSpacePosition - cameraPosition);

		Math::Plane translationPlane = Math::Plane::TroughPoint(m_Origin, TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f)));

		Math::Ray ray;
		ray.Direction = direction;
		ray.Origin = cameraPosition;

		float t = Math::IntersectPlane(translationPlane, ray);
		return cameraPosition + (glm::vec3)direction * t;
	}

	void EditorCamera::Drag(glm::vec2 mousePosition)
	{
		glm::mat4 inverseProjection = glm::inverse(m_ProjectionMatrix);
		glm::mat4 inverseView = glm::inverse(m_ViewMatrix);

		glm::vec3 previousPosition = CalculateTranslationPoint(mousePosition, inverseProjection, inverseView);
		glm::vec3 currentPosition = CalculateTranslationPoint(m_PreviousMousePosition, inverseProjection, inverseView);

		glm::vec3 movementDirection = (currentPosition - previousPosition);
		m_Origin += movementDirection;

		m_PreviousMousePosition = mousePosition;

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
