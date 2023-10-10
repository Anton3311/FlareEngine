#pragma once

#include "Flare.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/ComponentInitializer.h"

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	struct FLARE_API TransformComponent
	{
		FLARE_COMPONENT;

		TransformComponent();
		TransformComponent(const glm::vec3& position);
		
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
		SpriteComponent(AssetHandle texture);

		glm::vec4 Color;
		glm::vec2 TextureTiling;
		AssetHandle Texture;
		SpriteRenderFlags Flags;
	};

	struct FLARE_API SpriteLayer
	{
		FLARE_COMPONENT;

		SpriteLayer();
		SpriteLayer(int32_t layer);

		int32_t Layer;
	};

	struct FLARE_API MaterialComponent
	{
		FLARE_COMPONENT;

		MaterialComponent();
		MaterialComponent(AssetHandle handle);

		AssetHandle Material;
	};

	struct FLARE_API TextComponent
	{
		FLARE_COMPONENT;

		TextComponent();
		TextComponent(std::string_view text, const glm::vec4& color = glm::vec4(1.0f), AssetHandle font = NULL_ASSET_HANDLE);

		std::string Text;
		glm::vec4 Color;
		AssetHandle Font;
	};
}