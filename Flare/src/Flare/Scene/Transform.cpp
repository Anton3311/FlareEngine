#include "Transform.h"

namespace Flare
{
    FLARE_IMPL_COMPONENT(TransformComponent);
    glm::mat4 TransformComponent::GetTransformationMatrix() const
    {
        return glm::translate(glm::identity<glm::mat4>(), Position) * glm::toMat4(glm::quat(glm::radians(Rotation))) *
            glm::scale(glm::identity<glm::mat4>(), Scale);
    }

    glm::vec3 TransformComponent::TransformDirection(const glm::vec3& direction) const
    {
        return glm::rotate(glm::quat(glm::radians(Rotation)), direction);
    }



    FLARE_IMPL_COMPONENT(GlobalTransform);
    glm::mat4 GlobalTransform::GetTransformationMatrix() const
    {
        return glm::translate(glm::identity<glm::mat4>(), Position) * glm::toMat4(glm::quat(glm::radians(Rotation))) *
            glm::scale(glm::identity<glm::mat4>(), Scale);
    }

    glm::vec3 GlobalTransform::TransformDirection(const glm::vec3& direction) const
    {
        return glm::rotate(glm::quat(glm::radians(Rotation)), direction);
    }
}