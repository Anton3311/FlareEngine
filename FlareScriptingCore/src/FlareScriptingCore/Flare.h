#pragma once

#include "Flare/Core/UUID.h"

#include "FlareECS/ComponentId.h"
#include "FlareECS/EntityId.h"

#include "FlareScriptingCore/ECS/World.h"
#include "FlareScriptingCore/ECS/EntityView.h"
#include "FlareScriptingCore/ECS/SystemInfo.h"
#include "FlareScriptingCore/ECS/SystemConfiguration.h"
#include "FlareScriptingCore/ECS/ComponentInfo.h"
#include "FlareScriptingCore/ECS/Query.h"

#include "FlareScriptingCore/Time.h"
#include "FlareScriptingCore/Input.h"

#include "FlareScriptingCore/Texture.h"

#include "FlareScriptingCore/ScriptingType.h"

#include "FlareScriptingCore/TypeSerializationSettings.h"

namespace Flare
{
	using Scripting::World;
	using Scripting::EntityView;
	using Scripting::EntityElement;
	using Scripting::ComponentView;
	using Scripting::Query;

	using Scripting::ComponentInfo;
	using Scripting::ScriptingType;
	using Scripting::SystemBase;
	using Scripting::SystemInfo;
	using Scripting::SystemConfiguration;

	using Scripting::Time;
	using Scripting::Input;

	using Scripting::TextureAsset;

	using Scripting::TypeSerializationSettings;

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
