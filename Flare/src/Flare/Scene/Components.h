#pragma once

#include "Flare.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/ComponentInitializer.h"

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	struct FLARE_API TransformComponent
	{
		FLARE_COMPONENT;

		TransformComponent();

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;

		glm::mat4 GetTransformationMatrix() const;
	};

	struct FLARE_API CameraComponent
	{
		FLARE_COMPONENT;

		enum class ProjectionType : uint8_t
		{
			Orthographic,
			Perspective,
		};

		CameraComponent();

		glm::mat4 GetProjection() const;
		glm::vec3 ScreenToWorld(glm::vec2 point) const;
		glm::vec3 ViewportToWorld(glm::vec2 point) const;

		ProjectionType Projection;

		float Size;
		float FOV;
		float Near;
		float Far;
	};

	struct FLARE_API SpriteComponent
	{
		FLARE_COMPONENT;

		SpriteComponent();

		glm::vec4 Color;
		glm::vec2 TextureTiling;
		AssetHandle Texture;
	};
}