#pragma once

#include "FlareECS/World.h"

#include <glm/glm.hpp>

namespace Flare
{
	struct TransformComponent
	{
		FLARE_COMPONENT;

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;
	};

	struct CameraComponent
	{
		FLARE_COMPONENT;

		float Size;
		float Near;
		float Far;
	};

	struct SpriteComponent
	{
		FLARE_COMPONENT;

		glm::vec4 Color;
	};
}