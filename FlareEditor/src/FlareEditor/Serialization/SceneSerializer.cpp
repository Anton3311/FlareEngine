#include "SceneSerializer.h"

#include "Flare/Scene/Components.h"
#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Serialization/Serialization.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Query/EntitiesIterator.h"

#include "FlareEditor/Serialization/SerializationId.h"
#include "FlareEditor/Serialization/YAMLSerialization.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include <fstream>

namespace Flare
{
	void SceneSerializer::SerializeComponent(YAML::Emitter& emitter, const World& world, Entity entity, ComponentId component)
	{
		std::optional<const void*> entityData = world.Entities.GetEntityComponent(entity, component);
		const ComponentInfo& info = world.Components.GetComponentInfo(component);

		if (entityData.has_value() && info.Initializer)
		{
			YAMLSerializer serializer(emitter);
			emitter << YAML::BeginMap;
			emitter << YAML::Key << "Name" << YAML::Value << info.Initializer->Type.SerializationDescriptor.Name;

			emitter << YAML::Key << "Data" << YAML::Value;
			serializer.SerializeObject(info.Initializer->Type.SerializationDescriptor, (void*)entityData.value());
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

	Entity SceneSerializer::DeserializeEntity(const YAML::Node& node, World& world)
	{
		Entity entity;
		
		YAML::Node componentsNode = node["Components"];
		if (!componentsNode)
			return entity;

		for (YAML::Node componentNode : componentsNode)
		{
			std::string name;
			if (YAML::Node nameNode = componentNode["Name"])
				name = nameNode.as<std::string>();
			else
				continue;

			std::optional<ComponentId> componentId = world.Components.FindComponnet(name);

			if (!componentId.has_value())
			{
				FLARE_CORE_ERROR("Component named '{0}' cannot be deserialized", name);
				continue;
			}

			const ComponentInfo& info = world.Components.GetComponentInfo(componentId.value());
			if (info.Initializer)
			{
				const SerializableObjectDescriptor& serializationDescriptor = info.Initializer->Type.SerializationDescriptor;
				uint8_t* componentData = AddDeserializedComponent(world, entity, componentId.value());

				YAML::Node componentDataNode = componentNode["Data"];
				if (componentDataNode && !componentDataNode.IsNull())
				{
					YAMLDeserializer deserializer(componentNode);
					deserializer.PropertyKey("Data");
					deserializer.SerializeObject(serializationDescriptor, (void*)componentData);
				}
			}
		}

		if (!world.HasComponent<SerializationId>(entity))
			world.AddEntityComponent<SerializationId>(entity, SerializationId());

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

		emitter << YAML::EndSeq; // Entities

		emitter << YAML::Key << "PostProcessing" << YAML::BeginSeq;
		
		{
			Ref<ToneMapping> toneMapping = scene->GetPostProcessingManager().ToneMappingPass;
			const auto* descriptor = &FLARE_SERIALIZATION_DESCRIPTOR_OF(ToneMapping);

			emitter << YAML::Value << YAML::BeginMap;
			emitter << YAML::Key << "Name" << YAML::Value << descriptor->Name;

			YAMLSerializer serialzier(emitter);
			serialzier.Serialize("Data", SerializationValue(*toneMapping));

			emitter << YAML::EndMap;
		}

		{
			Ref<Vignette> vignette = scene->GetPostProcessingManager().VignettePass;
			const auto* descriptor = &FLARE_SERIALIZATION_DESCRIPTOR_OF(Vignette);

			emitter << YAML::Value << YAML::BeginMap;
			emitter << YAML::Key << "Name" << YAML::Value << descriptor->Name;

			YAMLSerializer serialzier(emitter);
			serialzier.Serialize("Data", SerializationValue(*vignette));

			emitter << YAML::EndMap;
		}

		emitter << YAML::EndMap; // PostProcessing

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
			DeserializeEntity(entity, scene->m_World);
		}

		YAML::Node postProcessing = node["PostProcessing"];
		if (!postProcessing)
			return;

		for (YAML::Node effectNode : postProcessing)
		{
			if (YAML::Node nameNode = effectNode["Name"])
			{
				std::string name = nameNode.as<std::string>();
				YAMLDeserializer deserializer(effectNode);
				if (name == ToneMapping::_Type.TypeName)
				{
					deserializer.Serialize("Data", SerializationValue(*scene->GetPostProcessingManager().ToneMappingPass));
				}
				else if (name == Vignette::_Type.TypeName)
				{
					deserializer.Serialize("Data", SerializationValue(*scene->GetPostProcessingManager().VignettePass));
				}
			}
		}
	}
}