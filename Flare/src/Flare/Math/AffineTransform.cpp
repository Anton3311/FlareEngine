#include "AffineTransform.h"

namespace Flare
{
	glm::mat4 Math::AffineTransform::GetTransformationMatrix() const
	{
		return glm::translate(glm::identity<glm::mat4>(), Position) * glm::toMat4(glm::quat(glm::radians(Rotation))) *
			glm::scale(glm::identity<glm::mat4>(), Scale);
	}

	glm::vec3 Math::AffineTransform::TransformDirection(const glm::vec3& direction) const
	{
		return glm::rotate(glm::quat(glm::radians(Rotation)), direction);
	}
}