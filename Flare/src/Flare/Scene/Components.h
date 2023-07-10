#pragma once

#include "Flare.h"

#include "FlareECS/World.h"

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	struct TransformComponent
	{
		FLARE_COMPONENT;

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;

		glm::mat4 GetTransformationMatrix() const;
	};

	struct CameraComponent
	{
		FLARE_COMPONENT;

		enum class ProjectionType : uint8_t
		{
			Orthographic,
			Perspective,
		};

		ProjectionType Projection;

		float Size;
		float FOV;
		float Near;
		float Far;
	};

	struct SpriteComponent
	{
		FLARE_COMPONENT;

		glm::vec4 Color;
		AssetHandle Texture;
	};
}