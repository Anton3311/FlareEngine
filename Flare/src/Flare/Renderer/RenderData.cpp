#include "RenderData.h"

namespace Flare
{
	void FrustumPlanes::SetFromViewAndProjection(const glm::mat4& view, const glm::mat4& inverseViewProjection, glm::vec3 viewDirection)
	{
		std::array<glm::vec4, 8> frustumCorners =
		{
			// Near
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f,  1.0f, 0.0f, 1.0f),

			// Far
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f,  1.0f, 1.0f, 1.0f),
		};

		for (size_t i = 0; i < frustumCorners.size(); i++)
		{
			frustumCorners[i] = inverseViewProjection * frustumCorners[i];
			frustumCorners[i] /= frustumCorners[i].w;
		}

		// Near
		Planes[FrustumPlanes::NearPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], viewDirection);

		// Far
		Planes[FrustumPlanes::FarPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[4], -viewDirection);

		// Left (Trough bottom left corner)
		Planes[FrustumPlanes::LeftPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], glm::normalize(glm::cross(
			// Bottom Left Near -> Bottom Left Far
			(glm::vec3)(frustumCorners[4] - frustumCorners[0]),
			// Bottom Left Near -> Top Left Near
			(glm::vec3)(frustumCorners[2] - frustumCorners[0]))));

		// Right (Trough top right corner)
		Planes[FrustumPlanes::RightPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[1], glm::cross(
			// Top Right Near -> Top Right Far
			(glm::vec3)(frustumCorners[7] - frustumCorners[3]),
			// Top Right Near -> Bottom Right Near
			(glm::vec3)(frustumCorners[1] - frustumCorners[3])));

		// Top (Trough top right corner)
		Planes[FrustumPlanes::TopPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[3], glm::cross(
			// Top Right Near -> Top Left Near
			(glm::vec3)(frustumCorners[2] - frustumCorners[3]),
			// Top Right Near -> Top Right Far
			(glm::vec3)(frustumCorners[7] - frustumCorners[3])));

		// Bottom (Trough bottom left corner)
		Planes[FrustumPlanes::BottomPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], glm::cross(
			// Bottom left near => Bottom right near
			(glm::vec3)(frustumCorners[1] - frustumCorners[0]),
			// Bottom left near -> Bottom left far
			(glm::vec3)(frustumCorners[4] - frustumCorners[0])));
	}

	void RenderView::SetViewAndProjection(const glm::mat4& projection, const glm::mat4& view)
	{
		Projection = projection;
		InverseProjection = glm::inverse(projection);

		View = view;
		InverseView = glm::inverse(view);

		ViewProjection = projection * view;
		InverseViewProjection = glm::inverse(ViewProjection);
	}
}
