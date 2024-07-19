#pragma once

#include "FlareCore/UUID.h"

#include "Flare/Scene/Scene.h"
#include "FlareECS/Entity/Entity.h"

#include "FlareEditor/EditorCamera.h"
#include "FlareEditor/SceneViewSettings.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <unordered_map>

namespace Flare
{
	class SceneSerializer
	{
	public:
		static void SerializeComponent(YAML::Emitter& emitter, const World& world, Entity entity, ComponentId component);

		static void SerializeEntity(YAML::Emitter& emitter, World& world, Entity entity);
		static void DeserializeEntity(Entity entity, const YAML::Node& node, World& world, std::unordered_map<UUID, Entity>& serializationIdToECSId);

		static void Serialize(const Ref<Scene>& scene,
			const EditorCamera& editorCamera,
			const SceneViewSettings& sceneViewSettings);

		static void Serialize(const Ref<Scene>& scene,
			const std::filesystem::path& path,
			const EditorCamera& editorCamera,
			const SceneViewSettings& sceneViewSettings);
		
		static void Deserialize(const Ref<Scene>& scene,
			const std::filesystem::path& path,
			EditorCamera& editorCamera,
			SceneViewSettings& sceneViewSettings);
	};
}