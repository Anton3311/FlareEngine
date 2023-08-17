#include "Components.h"

namespace Flare
{
	FLARE_IMPL_COMPONENT(TransformComponent);
	FLARE_IMPL_COMPONENT(CameraComponent);
	FLARE_IMPL_COMPONENT(SpriteComponent);

	glm::mat4 TransformComponent::GetTransformationMatrix() const
	{
		return glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(glm::quat(glm::radians(Rotation))) *
			glm::scale(glm::mat4(1.0f), Scale);
	}
}