#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare/Math/AffineTransform.h"

#include "FlareECS/Entity/ComponentInitializer.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Flare
{
	struct FLARE_API TransformComponent : public Math::AffineTransform
	{
		FLARE_COMPONENT;

		TransformComponent() = default;
		TransformComponent(const glm::vec3& position)
			: AffineTransform(position) {}
		TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
			: AffineTransform(position, rotation, scale) {}
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

	//
	// LocalTransform
	//

	struct FLARE_API LocalTransform : public Math::AffineTransform
	{
		FLARE_COMPONENT;

		LocalTransform() = default;
		LocalTransform(const glm::vec3& position)
			: AffineTransform(position) {}
		LocalTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
			: AffineTransform(position, rotation, scale) {}
	};

	template<>
	struct TypeSerializer<LocalTransform>
	{
		void OnSerialize(LocalTransform& transform, SerializationStream& stream)
		{
			stream.Serialize("Position", SerializationValue(transform.Position));
			stream.Serialize("Rotation", SerializationValue(transform.Rotation));
			stream.Serialize("Scale", SerializationValue(transform.Scale));
		}
	};
}
