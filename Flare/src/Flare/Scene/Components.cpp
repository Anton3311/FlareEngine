#include "Components.h"

namespace Flare
{
	FLARE_COMPONENT_IMPL(TransformComponent);
	FLARE_COMPONENT_IMPL(CameraComponent);
	FLARE_COMPONENT_IMPL(SpriteComponent);

	glm::mat4 TransformComponent::GetTransformationMatrix() const
	{
		return glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(glm::quat(glm::radians(Rotation))) *
			glm::scale(glm::mat4(1.0f), Scale);
	}
}