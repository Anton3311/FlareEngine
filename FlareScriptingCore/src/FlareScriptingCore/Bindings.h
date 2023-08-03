#pragma once

#include "Flare/Core/KeyCode.h"
#include "Flare/Core/MouseCode.h"

#include "FlareECS/ArchetypeId.h"
#include "FlareECS/ComponentId.h"
#include "FlareECS/EntityId.h"
#include "FlareECS/System.h"

#include <optional>
#include <functional>
#include <string_view>

namespace Flare::Internal
{
	class EntityView;
	struct Bindings
	{
		// ECS

		using CreateEntityFunction = Entity(*)(const ComponentId* components, size_t count);
		CreateEntityFunction CreateEntity;
		using GetEntityComponentFunction = void* (*)(Entity entity, ComponentId id);
		GetEntityComponentFunction GetEntityComponent;
		using IsEntityAliveFunction = bool(*)(Entity entity);
		IsEntityAliveFunction IsEntityAlive;

		using GetArchetypeComponentOffsetFunction = size_t(*)(ArchetypeId archetype, ComponentId component);
		GetArchetypeComponentOffsetFunction GetArchetypeComponentOffset;

		using FindSystemGroupFunction = std::optional<SystemGroupId>(*)(std::string_view name);
		FindSystemGroupFunction FindSystemGroup;

		// Input

		using IsKeyPressedFunction = bool(*)(KeyCode key);
		IsKeyPressedFunction IsKeyPressed;

		using IsMouseButtonPressedFunction = bool(*)(MouseCode button);
		IsMouseButtonPressedFunction IsMouseButtonPressed;

		static Bindings* Instance;
	};
}