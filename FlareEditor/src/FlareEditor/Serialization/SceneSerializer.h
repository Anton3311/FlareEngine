#pragma once

#include "Flare/Scene/Scene.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>

namespace Flare
{
	class SceneSerializer
	{
	public:
		static void SerializeComponent(YAML::Emitter& emitter, const World& world, Entity entity, ComponentId component);

		static void SerializeEntity(YAML::Emitter& emitter, World& world, Entity entity);
		static Entity DeserializeEntity(YAML::Node node, World& world);

		static void Serialize(const Ref<Scene>& scene);
		static void Serialize(const Ref<Scene>& scene, const std::filesystem::path& path);
		static void Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path);
	};
}