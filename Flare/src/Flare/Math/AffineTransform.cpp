#include "PCH.h"

#include "AffineTransform.h"

namespace Flare
{
	void Math::AffineTransform::ApplyTransform(const AffineTransform& transform)
	{
		glm::vec3 rotatedPosition = glm::quat(glm::radians(transform.Rotation)) * (Position * transform.Scale);

		Position = rotatedPosition + transform.Position;
		Rotation = transform.Rotation + Rotation;
		Scale = transform.Scale * Scale;
	}

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