#pragma once

#include "Flare/Core/UUID.h"

#include "FlareScriptingCore/Bindings/ECS/Component.h"
#include "FlareScriptingCore/Bindings/ECS/World.h"
#include "FlareScriptingCore/Bindings/ECS/EntityView.h"

#include "FlareScriptingCore/Bindings/Time.h"

#include "FlareScriptingCore/ComponentInfo.h"
#include "FlareScriptingCore/SystemInfo.h"
#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/SystemConfiguration.h"

#include "FlareScriptingCore/TypeSerializationSettings.h"

namespace Flare
{
	using Internal::World;
	using Internal::Entity;
	using Internal::EntityView;
	using Internal::EntityElement;
	using Internal::ComponentView;

	using Internal::ComponentInfo;
	using Internal::ScriptingType;
	using Internal::SystemBase;
	using Internal::SystemInfo;
	using Internal::SystemConfiguration;

	using Internal::Time;

	using Internal::TypeSerializationSettings;

	struct Transform
	{
		FLARE_COMPONENT(Transform);

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;
	};
	FLARE_COMPONENT_ALIAS_IMPL(Transform, "struct Flare::TransformComponent");

	struct Sprite
	{
		FLARE_COMPONENT(Sprite);

		glm::vec4 Color;
		glm::vec2 TextureTiling;

		UUID Texture;
	};
	FLARE_COMPONENT_ALIAS_IMPL(Sprite, "struct Flare::SpriteComponent");
}
