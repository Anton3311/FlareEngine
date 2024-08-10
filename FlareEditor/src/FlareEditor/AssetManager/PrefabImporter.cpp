#include "PrefabImporter.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Prefab.h"

#include "FlareECS/World.h"

#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/Serialization/SceneSerializer.h"
#include "FlareEditor/Serialization/YAMLSerialization.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <exception>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	static void SerializeNodeComponents(YAML::Emitter& emitter, const PrefabHierarchy& hierarchy, size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		uint8_t* entityData = hierarchy.GetEntityData(nodeIndex);

		const PrefabHierarchy::Node& node = hierarchy.GetNodes()[nodeIndex];
		const ArchetypeRecord& archetype = hierarchy.GetCompatibleArchetypes()[node.Archetype];

		for (size_t componentIndex = 0; componentIndex < archetype.Components.size(); componentIndex++)
		{
			const ComponentInfo& info = hierarchy.GetCompatibleComponents().GetComponentInfo(archetype.Components[componentIndex]);

			YAMLSerializer serializer(emitter, nullptr);
			emitter << YAML::BeginMap;
			emitter << YAML::Key << "Name" << YAML::Value << info.Initializer->SerializationDescriptor.Name;

			emitter << YAML::Key << "Data" << YAML::Value;
			serializer.SerializeObject(info.Initializer->SerializationDescriptor, entityData + archetype.ComponentOffsets[componentIndex], false, 0);
			emitter << YAML::EndMap;
		}

	}

	static void SerializePrefabHierarchy(YAML::Emitter& emitter, const PrefabHierarchy& hierarachy)
	{
		FLARE_PROFILE_FUNCTION();

		emitter << YAML::BeginSeq;

		const auto& nodes = hierarachy.GetNodes();
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			emitter << YAML::BeginMap;

			emitter << YAML::Key << "Components" << YAML::Value << YAML::BeginSeq;
			SerializeNodeComponents(emitter, hierarachy, nodeIndex);
			emitter << YAML::EndSeq;

			emitter << YAML::EndMap;
		}

		emitter << YAML::EndSeq;
	}

	void PrefabImporter::SerializePrefab(AssetHandle prefab, World& world, Entity entity)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(prefab));

		YAML::Emitter emitter;
		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(prefab);
		Ref<Prefab> prefabAsset = AssetManager::GetAsset<Prefab>(prefab);

		SerializePrefabHierarchy(emitter, prefabAsset->GetHierarchy());

		std::ofstream output(metadata->Path);
		output << emitter.c_str();
		output.close();
	}

	static void DeserializePrefabHierarchy(const YAML::Node& root, PrefabHierarchy& hierarchy)
	{
		FLARE_PROFILE_FUNCTION();

		struct EntityDataNode
		{
			std::vector<const ComponentInfo*> ComponentIds;
			std::vector<YAML::Node> ComponentNodes;
			ArchetypeId Archetype = INVALID_ARCHETYPE_ID;
		};

		std::vector<EntityDataNode> dataNodes;

		const Components& componentsRegistry = hierarchy.GetCompatibleComponents();

		for (YAML::Node entityNode : root)
		{
			YAML::Node components = entityNode["Components"];
			if (!components)
				continue;

			EntityDataNode& dataNode = dataNodes.emplace_back();
			std::vector<ComponentId> ids;

			for (YAML::Node componentNode : components)
			{
				if (YAML::Node name = componentNode["Name"])
				{
					std::string componentName = name.as<std::string>();
					std::optional<ComponentId> componentId = componentsRegistry.FindComponnet(componentName);

					if (componentId)
					{
						dataNode.ComponentIds.push_back(&componentsRegistry.GetComponentInfo(*componentId));
						ids.push_back(*componentId);
						dataNode.ComponentNodes.push_back(componentNode);
					}
					else
					{
						FLARE_CORE_WARN("PrefabImporter: Component '{}' cannot be found", componentName);
					}
				}
				else
					continue;
			}

			const ArchetypeRecord* archetype = hierarchy.GetCompatibleArchetypes().FindOrCreateArchetype(Span<ComponentId>::FromVector(ids));
			FLARE_CORE_ASSERT(archetype);

			dataNode.Archetype = archetype->Id;

			hierarchy.AddEntity(archetype->Id);
		}

		hierarchy.EnsureAllocated();

		for (size_t nodeIndex = 0; nodeIndex < dataNodes.size(); nodeIndex++)
		{
			const auto& node = dataNodes[nodeIndex];
			uint8_t* entityData = hierarchy.GetEntityData(nodeIndex);

			for (size_t componentIndex = 0; componentIndex < node.ComponentIds.size(); componentIndex++)
			{
				const ComponentInfo* component = node.ComponentIds[componentIndex];
				const ArchetypeRecord& archetype = hierarchy.GetCompatibleArchetypes()[node.Archetype];

				std::optional<size_t> archetypeComponentIndex = archetype.TryGetComponentIndex(component->Id);
				FLARE_CORE_ASSERT(archetypeComponentIndex);

				uint8_t* componentData = entityData + archetype.ComponentOffsets[*archetypeComponentIndex];

				component->Initializer->Type.DefaultConstructor(componentData);

				YAMLDeserializer deserializer(node.ComponentNodes[componentIndex], nullptr);
				deserializer.PropertyKey("Data");
				deserializer.SerializeObject(component->Initializer->SerializationDescriptor, componentData, false, 0);
			}
		}
	}

	Ref<Asset> PrefabImporter::ImportPrefab(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<std::pair<ComponentId, void*>> components;

		ECSContext& context = EditorLayer::GetInstance().GetECSContext();
		Ref<Prefab> prefab = CreateRef<Prefab>(context.Components, context.Archetypes);

		YAML::Node node = YAML::LoadFile(metadata.Path.generic_string());
		DeserializePrefabHierarchy(node, prefab->GetHierarchy());

		return prefab;
#if 0

		try
		{
			YAML::Node node = YAML::LoadFile(metadata.Path.generic_string());
			
			World& world = World::GetCurrent();

			size_t prefabDataSize = 0;
			std::vector<std::pair<ComponentId, void*>> components;

			if (YAML::Node componentsNode = node["Components"])
			{
				for (YAML::Node componentNode : componentsNode)
				{
					if (YAML::Node nameNode = componentNode["Name"])
					{
						std::string name = nameNode.as<std::string>();
						auto componentId = world.Components.FindComponnet(name);

						if (!componentId.has_value())
							continue;

						if (componentId.has_value())
						{
							prefabDataSize += world.Components.GetComponentInfo(componentId.value()).Size;
							components.push_back({ componentId.value(), nullptr });
						}
					}
				}
			}

			uint8_t* prefabData = nullptr;
			if (prefabDataSize != 0)
			{
				prefabData = new uint8_t[prefabDataSize];
				std::memset(prefabData, 0, prefabDataSize);

				size_t writeOffset = 0;
				size_t index = 0;

				if (YAML::Node componentsNode = node["Components"])
				{
					for (YAML::Node componentNode : componentsNode)
					{
						YAML::Node nameNode = componentNode["Name"];
						if (!nameNode)
							continue;

						std::string name = nameNode.as<std::string>();
						auto componentId = world.Components.FindComponnet(name);
						if (!componentId)
							continue;

						const ComponentInfo& info = world.Components.GetComponentInfo(componentId.value());

						YAML::Node dataNode = componentNode["Data"];
						if (info.Initializer && dataNode)
						{
							YAMLDeserializer deserializer(componentNode);
							deserializer.PropertyKey("Data");
							deserializer.SerializeObject(info.Initializer->SerializationDescriptor, prefabData + writeOffset, false, 0);
						}

						components[index].second = (void*)(prefabData + writeOffset);

						writeOffset += info.Size;
						index++;
					}
				}
			}

			using Pair = std::pair<ComponentId, void*>;
			std::sort(components.begin(), components.end(), [](const Pair& a, const Pair& b) -> bool { return a.first < b.first; });

			return CreateRef<Prefab>(prefabData, world.Components, world.GetArchetypes(), std::move(components));
		}
		catch (std::exception& e)
		{
			FLARE_CORE_ERROR("Failed to import prefab '{}': {}", metadata.Path.generic_string(), e.what());
		}

		return nullptr;
#endif
	}
}
