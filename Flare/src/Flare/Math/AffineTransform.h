#pragma once

#include "FlareCore/Core.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Flare::Math
{
	struct Compact3DTransform
	{
	public:
		Compact3DTransform() = default;
		Compact3DTransform(const glm::mat3& rotationScale, const glm::vec3& translation)
			: RotationScale(rotationScale), Translation(translation) {}

		explicit Compact3DTransform(const glm::mat4& transformationMatrix)
		{
			RotationScale = transformationMatrix;
			Translation = transformationMatrix[3];
		}

		glm::vec3 TransformDirection(glm::vec3 direction) const
		{
			return RotationScale * direction;
		}

		glm::mat4 ToMatrix4x4() const
		{
			return glm::mat4(
				glm::vec4(RotationScale[0], 0.0f),
				glm::vec4(RotationScale[1], 0.0f),
				glm::vec4(RotationScale[2], 0.0f),
				glm::vec4(Translation, 1.0f));
		}
	public:
		glm::mat3 RotationScale = glm::mat3(1.0f);
		glm::vec3 Translation = glm::vec3(0.0f);
	};

	//
	// AffineTransform
	//

    struct FLARE_API AffineTransform
    {
		AffineTransform()
			: Position(glm::vec3(0.0f)),
			Rotation(glm::vec3(0.0f)),
			Scale(glm::vec3(1.0f)) {}

		AffineTransform(const glm::vec3& position)
			: Position(position), Rotation(glm::vec3(0.0f)), Scale(glm::vec3(1.0f)) {}

		AffineTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
			: Position(position), Rotation(rotation), Scale(scale) {}

		// Transforms this transform by applying the given one
		void ApplyTransform(const AffineTransform& transform);
        
		glm::mat4 GetTransformationMatrix() const;
		glm::vec3 TransformDirection(const glm::vec3& direction) const;

        glm::vec3 Position;
        glm::vec3 Rotation;
        glm::vec3 Scale;
    };
}
