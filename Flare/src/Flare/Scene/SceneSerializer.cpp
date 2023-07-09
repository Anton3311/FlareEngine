#include "SceneSerializer.h"

#include "Flare/Scene/Components.h"
#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Serialization/Serialization.h"

#include "FlareECS/Query/EntityRegistryIterator.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include <fstream>

namespace Flare
{
	static void SerializeComponent(YAML::Emitter& emitter, World& world, Entity entity, ComponentId component)
	{
		if (component == TransformComponent::Id)
		{
			TransformComponent& transform = world.GetEntityComponent<TransformComponent>(entity);

			emitter << YAML::Key << "Transform" << YAML::BeginMap;

			emitter << YAML::Key << "Position" << YAML::Value << transform.Position;
			emitter << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			emitter << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			emitter << YAML::EndMap;
		}
		else if (component == SpriteComponent::Id)
		{
			SpriteComponent& sprite = world.GetEntityComponent<SpriteComponent>(entity);

			emitter << YAML::Key << "Sprite" << YAML::BeginMap;
			emitter << YAML::Key << "Color" << YAML::Value << sprite.Color;
			emitter << YAML::Key << "Texture" << YAML::Value << sprite.Texture;
			emitter << YAML::EndMap;
		}
		else if (component == CameraComponent::Id)
		{
			CameraComponent& camera = world.GetEntityComponent<CameraComponent>(entity);

			emitter << YAML::Key << "Camera" << YAML::BeginMap;
			emitter << YAML::Key << "Size" << YAML::Value << camera.Size;
			emitter << YAML::Key << "Near" << YAML::Value << camera.Near;
			emitter << YAML::Key << "Far" << YAML::Value << camera.Far;
			emitter << YAML::EndMap;
		}
	}

	template<typename T>
	static void AddDeserializedComponent(World& world, Entity& entity, const T& componentData)
	{
		if (world.IsEntityAlive(entity))
			world.AddEntityComponent<T>(entity, componentData);
		else
		{
			entity = world.CreateEntity<T>();
			T& component = world.GetEntityComponent<T>(entity);
			component = componentData;
		}
	}

	void SceneSerializer::Serialize(const Ref<Scene>& scene)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(scene->Handle));
		const std::filesystem::path& path = AssetManager::GetAssetMetadata(scene->Handle)->Path;

		YAML::Emitter emitter;
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Entities";
		emitter << YAML::BeginSeq;

		for (Entity entity : scene->m_World.GetRegistry())
		{
			emitter << YAML::BeginMap;

			emitter << YAML::Key << "Components" << YAML::BeginMap; // Components

			for (ComponentId component : scene->m_World.GetEntityComponents(entity))
				SerializeComponent(emitter, scene->m_World, entity, component);

			emitter << YAML::EndMap; // Components
			emitter << YAML::EndMap;
		}

		emitter << YAML::EndSeq;
		emitter << YAML::EndMap;

		std::ofstream output(path);
		if (!output.is_open())
		{
			FLARE_CORE_ERROR("Failed to write scene file: {0}", path.string());
			return;
		}

		output << emitter.c_str();
		output.close();
	}

	void SceneSerializer::Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path)
	{
		std::ifstream inputFile(path);
		if (!inputFile)
		{
			FLARE_CORE_ERROR("Failed to read scene file: {0}", path.string());
			return;
		}

		YAML::Node node = YAML::Load(inputFile);
		inputFile.close();

		YAML::Node entities = node["Entities"];
		if (!entities)
			return;

		for (auto entity : entities)
		{
			YAML::Node componentsNode = entity["Components"];

			Entity entity;
			if (componentsNode)
			{
				if (componentsNode["Transform"])
				{
					YAML::Node transformNode = componentsNode["Transform"];

					YAML::Node positionNode = transformNode["Position"];
					
					TransformComponent transformComponent;
					transformComponent.Position = positionNode.as<glm::vec3>();
					transformComponent.Rotation = transformNode["Rotation"].as<glm::vec3>();
					transformComponent.Scale = transformNode["Scale"].as<glm::vec3>();

					AddDeserializedComponent<TransformComponent>(scene->m_World, entity, transformComponent);
				}
				
				if (componentsNode["Sprite"])
				{
					YAML::Node spriteNode = componentsNode["Sprite"];

					SpriteComponent spriteComponent;
					spriteComponent.Color = spriteNode["Color"].as<glm::vec4>();

					YAML::Node textureNode = spriteNode["Texture"];
					spriteComponent.Texture = textureNode ? textureNode.as<uint64_t>() : NULL_ASSET_HANDLE;

					AddDeserializedComponent<SpriteComponent>(scene->m_World, entity, spriteComponent);
				}

				if (componentsNode["Camera"])
				{
					YAML::Node cameraNode = componentsNode["Camera"];

					CameraComponent cameraComponent;
					cameraComponent.Size = cameraNode["Size"].as<float>();
					cameraComponent.Near = cameraNode["Near"].as<float>();
					cameraComponent.Far = cameraNode["Far"].as<float>();

					AddDeserializedComponent<CameraComponent>(scene->m_World, entity, cameraComponent);
				}
			}
		}
	}
}