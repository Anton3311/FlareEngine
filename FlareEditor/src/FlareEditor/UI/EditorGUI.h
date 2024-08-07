#pragma once

#include "Flare.h"

#include "Flare/Scripting/ScriptingEngine.h"

#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/World.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorIcons.h"

namespace Flare
{
	constexpr char* ENTITY_PAYLOAD_NAME = "ENTITY_PAYLOAD";

	class EditorGUI
	{
	public:
		static void Initialize();
		static void Uninitialize();

		static bool BeginPropertyGrid(float width = 0.0f);
		static void EndPropertyGrid();

		static void DrawIcon(glm::ivec2 iconPosition, float size = 0.0f);
		static const EditorIcons& GetIcons();

		static void MoveCursor(ImVec2 offset);

		static bool BeginMenu(const char* name);
		static void EndMenu();

		static bool ObjectField(const SerializableObjectDescriptor& descriptor, void* data, const World* currentWorld = nullptr);

		static bool BoolPropertyField(const char* name, bool& value);
		static bool IntPropertyField(const char* name, int32_t& value);
		static bool UIntPropertyField(const char* name, uint32_t& value);

		static bool IntVector2PropertyField(const char* name, glm::ivec2& value);

		static bool FloatPropertyField(const char* name, float& value);
		static bool Vector2PropertyField(const char* name, glm::vec2& value);
		static bool Vector3PropertyField(const char* name, glm::vec3& value);
		static bool Vector4PropertyField(const char* name, glm::vec4& value);

		static bool ColorPropertyField(const char* name, glm::vec4& color);
		static bool ColorPropertyField(const char* name, glm::vec3& color);

		static bool TextProperty(const char* name, std::string& text);
		static bool TextField(const char* name, std::string& text);
		static bool TextField(std::string& text);
		static bool TextField(UUID id, std::string& text);

		static void UUIDField(const char* name, UUID uuid);
		static bool EntityField(const char* name, const World& world, Entity& entity);
		static bool EntityField(const World& world, Entity& entity);

		static bool BeginToggleGroup(const char* name, uint32_t itemsCount);
		static bool BeginToggleGroupProperty(const char* name, uint32_t itemsCount);
		static bool ToggleGroupItem(const char* text, bool selected);
		static void EndToggleGroup();

		static bool AssetField(AssetHandle& handle, const AssetDescriptor* assetType = nullptr);
		static bool AssetField(Ref<Asset>& asset, const AssetDescriptor& assetDescriptor);

		static bool AssetField(const char* name, Ref<Asset>& asset, const AssetDescriptor& assetDescriptor);
		static bool AssetField(const char* name, AssetHandle& handle, const AssetDescriptor* assetType = nullptr);

		static void PropertyName(const char* name, float minHeight = 0.0f);
		static void PropertyIndex(size_t index);
	};
}