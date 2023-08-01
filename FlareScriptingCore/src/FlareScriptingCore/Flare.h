#pragma once

#include "Flare/Core/UUID.h"

#include "FlareECS/ComponentId.h"
#include "FlareECS/EntityId.h"

#include "FlareScriptingCore/Bindings/ECS/World.h"
#include "FlareScriptingCore/Bindings/ECS/EntityView.h"
#include "FlareScriptingCore/Bindings/ECS/SystemInfo.h"
#include "FlareScriptingCore/Bindings/ECS/SystemConfiguration.h"
#include "FlareScriptingCore/Bindings/ECS/ComponentInfo.h"

#include "FlareScriptingCore/Bindings/Time.h"
#include "FlareScriptingCore/Bindings/Input.h"

#include "FlareScriptingCore/Bindings/Texture.h"

#include "FlareScriptingCore/ScriptingType.h"

#include "FlareScriptingCore/TypeSerializationSettings.h"

namespace Flare
{
	using Internal::World;
	using Internal::EntityView;
	using Internal::EntityElement;
	using Internal::ComponentView;

	using Internal::ComponentInfo;
	using Internal::ScriptingType;
	using Internal::SystemBase;
	using Internal::SystemInfo;
	using Internal::SystemConfiguration;

	using Internal::Time;
	using Internal::Input;

	using Internal::TextureAsset;

	using Internal::TypeSerializationSettings;

	struct Transform
	{
		FLARE_COMPONENT(Transform);

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;
	};

	struct Sprite
	{
		FLARE_COMPONENT(Sprite);

		glm::vec4 Color;
		glm::vec2 TextureTiling;

		TextureAsset Texture;
	};
}
