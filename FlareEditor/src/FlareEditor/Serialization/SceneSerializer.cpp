#include "SceneSerializer.h"

#include "Flare/Scene/Components.h"
#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Serialization/Serialization.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Query/EntitiesIterator.h"

#include "FlareEditor/Serialization/Serialization.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include <fstream>

namespace Flare
{
	void SceneSerializer::SerializeComponent(YAML::Emitter& emitter, const World& world, Entity entity, ComponentId component)
	{
		if (component == COMPONENT_ID(TransformComponent))
		{
			const TransformComponent& transform = world.GetEntityComponent<TransformComponent>(entity);

			emitter << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << "Transform";

			emitter << YAML::Key << "Position" << YAML::Value << transform.Position;
			emitter << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			emitter << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			emitter << YAML::EndMap;
		}
		else if (component == COMPONENT_ID(SpriteComponent))
		{
			const SpriteComponent& sprite = world.GetEntityComponent<SpriteComponent>(entity);

			emitter << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << "Sprite";
			emitter << YAML::Key << "Color" << YAML::Value << sprite.Color;
			emitter << YAML::Key << "Texture" << YAML::Value << sprite.Texture;
			emitter << YAML::Key << "TextureTiling" << YAML::Value << sprite.TextureTiling;
			emitter << YAML::Key << "Flags" << YAML::Value << (uint64_t)sprite.Flags;
			emitter << YAML::EndMap;
		}
		else if (component == COMPONENT_ID(CameraComponent))
		{
			const CameraComponent& camera = world.GetEntityComponent<CameraComponent>(entity);

			emitter << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << "Camera";

			emitter << YAML::Key << "Size" << YAML::Value << camera.Size;
			emitter << YAML::Key << "Near" << YAML::Value << camera.Near;
			emitter << YAML::Key << "Far" << YAML::Value << camera.Far;

			emitter << YAML::Key << "Projection";
			switch (camera.Projection)
			{
			case CameraComponent::ProjectionType::Orthographic:
				emitter << YAML::Value << "Orthographic";
				break;
			case CameraComponent::ProjectionType::Perspective:
				emitter << YAML::Value << "Perspective";
				break;
			}

			emitter << YAML::Key << "FOV" << YAML::Value << camera.FOV;
			emitter << YAML::EndMap;
		}
		else
		{
			std::optional<const void*> entityData = world.Entities.GetEntityComponent(entity, component);
			const ComponentInfo& info = world.Components.GetComponentInfo(component);

			if (entityData.has_value() && info.Initializer)
				SerializeType(emitter, info.Initializer->Type, (const uint8_t*)entityData.value());
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

	static uint8_t* AddDeserializedComponent(World& world, Entity& entity, ComponentId id)
	{
		if (world.IsEntityAlive(entity))
		{
			world.Entities.AddEntityComponent(entity, id, nullptr);
			return (uint8_t*)world.Entities.GetEntityComponent(entity, id).value();
		}
		else
		{
			entity = world.Entities.CreateEntity(ComponentSet(&id, 1));
			return (uint8_t*)world.Entities.GetEntityComponent(entity, id).value();
		}
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& emitter, World& world, Entity entity)
	{
		emitter << YAML::BeginMap;

		emitter << YAML::Key << "Components" << YAML::BeginSeq; // Components

		for (ComponentId component : world.GetEntityComponents(entity))
			SerializeComponent(emitter, world, entity, component);

		emitter << YAML::EndSeq; // Components
		emitter << YAML::EndMap;
	}

	Entity SceneSerializer::DeserializeEntity(YAML::Node node, World& world)
	{
		Entity entity;
		for (YAML::Node componentNode : node)
		{
			std::string name = componentNode["Name"].as<std::string>();

			if (name == "Transform")
			{
				YAML::Node positionNode = componentNode["Position"];

				TransformComponent transformComponent;
				transformComponent.Position = positionNode.as<glm::vec3>();
				transformComponent.Rotation = componentNode["Rotation"].as<glm::vec3>();
				transformComponent.Scale = componentNode["Scale"].as<glm::vec3>();

				AddDeserializedComponent<TransformComponent>(world, entity, transformComponent);
			}
			else if (name == "Sprite")
			{
				SpriteComponent spriteComponent;
				spriteComponent.Color = componentNode["Color"].as<glm::vec4>();

				if (YAML::Node textureNode = componentNode["Texture"])
					spriteComponent.Texture = textureNode ? textureNode.as<AssetHandle>() : NULL_ASSET_HANDLE;

				if (YAML::Node tilingNode = componentNode["TextureTiling"])
					spriteComponent.TextureTiling = tilingNode.as<glm::vec2>();
				else
					spriteComponent.TextureTiling = glm::vec2(1.0f);

				if (YAML::Node flags = componentNode["Flags"])
					spriteComponent.Flags = (SpriteRenderFlags)flags.as<uint64_t>();
				else
					spriteComponent.Flags = SpriteRenderFlags::None;

				AddDeserializedComponent<SpriteComponent>(world, entity, spriteComponent);
			}
			else if (name == "Camera")
			{
				CameraComponent cameraComponent;
				cameraComponent.Size = componentNode["Size"].as<float>();
				cameraComponent.Near = componentNode["Near"].as<float>();
				cameraComponent.Far = componentNode["Far"].as<float>();

				if (YAML::Node fovNode = componentNode["FOV"])
					cameraComponent.FOV = fovNode.as<float>();

				if (YAML::Node projectionType = componentNode["Projection"])
				{
					std::string string = projectionType.as<std::string>();
					if (string == "Orthographic")
						cameraComponent.Projection = CameraComponent::ProjectionType::Orthographic;
					else if (string == "Perspective")
						cameraComponent.Projection = CameraComponent::ProjectionType::Perspective;
				}

				AddDeserializedComponent<CameraComponent>(world, entity, cameraComponent);
			}
			else
			{
				std::optional<ComponentId> componentId = world.Components.FindComponnet(name);
				if (!componentId.has_value())
				{
					FLARE_CORE_ERROR("Component named '{0}' cannot be deserialized", name);
					continue;
				}

				const ComponentInfo& info = world.Components.GetComponentInfo(componentId.value());
				if (info.Initializer)
				{
					uint8_t* componentData = AddDeserializedComponent(world, entity, componentId.value());
					DeserializeType(componentNode, info.Initializer->Type, componentData);
				}
			}
		}

		return entity;
	}

	void SceneSerializer::Serialize(const Ref<Scene>& scene)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(scene->Handle));
		Serialize(scene, AssetManager::GetAssetMetadata(scene->Handle)->Path);
	}

	void SceneSerializer::Serialize(const Ref<Scene>& scene, const std::filesystem::path& path)
	{
		YAML::Emitter emitter;
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Entities";
		emitter << YAML::BeginSeq;

		for (Entity entity : scene->m_World.Entities)
			SerializeEntity(emitter, scene->m_World, entity);

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

		YAML::Node node;
		try
		{
			node = YAML::Load(inputFile);
		}
		catch (std::exception& e)
		{
			FLARE_CORE_ERROR(e.what());
			inputFile.close();
			return;
		}

		YAML::Node entities = node["Entities"];
		if (!entities)
			return;

		for (auto entity : entities)
		{
			YAML::Node componentsNode = entity["Components"];

			DeserializeEntity(componentsNode, scene->m_World);
		}
	}
}