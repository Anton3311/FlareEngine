#pragma once

#include "FlareCore/Core.h"

#include <glm/glm.hpp>

namespace Flare::Math
{
	FLARE_API bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

	struct Plane
	{
		inline static Plane TroughPoint(glm::vec3 point, glm::vec3 planeNormmal)
		{
			return { planeNormmal, -glm::dot(point, planeNormmal)};
		}

		glm::vec3 Normal;
		float Offset;
	};

	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

	inline float IntersectPlane(glm::vec3 planeNormal, glm::vec3 rayDirection, glm::vec3 rayOrigin)
	{
		return -glm::dot(planeNormal, rayOrigin) / glm::dot(rayDirection, planeNormal);
	}

	inline float IntersectPlane(const Plane& plane, const Ray& ray)
	{
		return (-plane.Offset - glm::dot(plane.Normal, ray.Origin)) / glm::dot(plane.Normal, ray.Direction);
	}
}