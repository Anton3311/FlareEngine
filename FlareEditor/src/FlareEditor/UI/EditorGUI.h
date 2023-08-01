#pragma once

#include "Flare.h"

#include "FlareECS/EntityId.h"
#include "FlareECS/World.h"

namespace Flare
{
	constexpr char* ENTITY_PAYLOAD_NAME = "ENTITY_PAYLOAD";

	class EditorGUI
	{
	public:
		static bool BeginPropertyGrid();
		static void EndPropertyGrid();

		static bool BeginMenu(const char* name);
		static void EndMenu();

		static bool FloatPropertyField(const char* name, float& value);
		static bool Vector2PropertyField(const char* name, glm::vec2& value);
		static bool Vector3PropertyField(const char* name, glm::vec3& value);
		static bool ColorPropertyField(const char* name, glm::vec4& color);

		static bool AssetField(const char* name, AssetHandle& handle);
		static bool EntityField(const char* name, const World& world, Entity& entity);

		static bool BeginToggleGroup(const char* name);
		static bool ToggleGroupItem(const char* text, bool selected);
		static void EndToggleGroup();
	private:
		static void RenderPropertyName(const char* name);
	};
}