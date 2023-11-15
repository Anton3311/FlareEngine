#pragma once

#include "FlareCore/Core.h"

#include <glm/glm.hpp>

namespace Flare::Math
{
	FLARE_API bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

	inline float IntersectPlane(glm::vec3 planeNormal, glm::vec3 rayDirection, glm::vec3 rayOrigin)
	{
		return -glm::dot(planeNormal, rayOrigin) / glm::dot(rayDirection, planeNormal);
	}
}