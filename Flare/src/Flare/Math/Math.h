#pragma once

#include "FlareCore/Core.h"

#include <glm/glm.hpp>

namespace Flare::Math
{
	FLARE_API bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);
}