#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "FlareECS/Entity/ComponentInitializer.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Flare
{
    struct FLARE_API TransformComponent
    {
        FLARE_COMPONENT;

		TransformComponent()
			: Position(glm::vec3(0.0f)),
			Rotation(glm::vec3(0.0f)),
			Scale(glm::vec3(1.0f)) {}

		TransformComponent(const glm::vec3& position)
			: Position(position), Rotation(glm::vec3(0.0f)), Scale(glm::vec3(1.0f)) {}

		TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
			: Position(position), Rotation(rotation), Scale(scale) {}
        
        glm::mat4 GetTransformationMatrix() const;
        glm::vec3 TransformDirection(const glm::vec3& direction) const;

        glm::vec3 Position;
        glm::vec3 Rotation;
        glm::vec3 Scale;
    };

    template<>
    struct TypeSerializer<TransformComponent>
    {
        void OnSerialize(TransformComponent& transform, SerializationStream& stream)
        {
            stream.Serialize("Position", SerializationValue(transform.Position));
            stream.Serialize("Rotation", SerializationValue(transform.Rotation));
            stream.Serialize("Scale", SerializationValue(transform.Scale));
        }
    };



    struct FLARE_API GlobalTransform
    {
        FLARE_COMPONENT;

		GlobalTransform()
			: Position(glm::vec3(0.0f)),
			Rotation(glm::vec3(0.0f)),
			Scale(glm::vec3(1.0f)) {}

		GlobalTransform(const glm::vec3& position)
			: Position(position), Rotation(glm::vec3(0.0f)), Scale(glm::vec3(1.0f)) {}

		GlobalTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
			: Position(position), Rotation(rotation), Scale(scale) {}
        
        glm::mat4 GetTransformationMatrix() const;
        glm::vec3 TransformDirection(const glm::vec3& direction) const;

        glm::vec3 Position;
        glm::vec3 Rotation;
        glm::vec3 Scale;
    };

    template<>
    struct TypeSerializer<GlobalTransform>
    {
        void OnSerialize(GlobalTransform& transform, SerializationStream& stream)
        {
            stream.Serialize("Position", SerializationValue(transform.Position));
            stream.Serialize("Rotation", SerializationValue(transform.Rotation));
            stream.Serialize("Scale", SerializationValue(transform.Scale));
        }
    };
}
