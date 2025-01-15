#include "PCH.h"

#include "PrefabImporter.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Prefab.h"
#include "Flare/Serialization/Serialization.h"

#include "FlareECS/World.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/AssetManager/MeshImporter.h"

#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/Serialization/SceneSerializer.h"
#include "FlareEditor/Serialization/YAMLSerialization.h"

#include <exception>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	inline static size_t Align(size_t value, size_t alignment)
	{
		return (value + alignment - 1) / alignment * alignment;
	}

	inline static size_t ComputeNextComponentOffset(size_t& currentOffset, const ComponentInfo& componentInfo)
	{
		currentOffset = Align(currentOffset, componentInfo.Initializer->Type.Alignment);
		size_t componentOffset = currentOffset;
		currentOffset += componentInfo.Size;
		return componentOffset;
	}

	static void SerializeNodeComponents(YAML::Emitter& emitter, const PrefabHierarchy& hierarchy, size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		uint8_t* entityData = hierarchy.GetEntityData(nodeIndex);

		const PrefabHierarchy::Node& node = hierarchy.GetNodes()[nodeIndex];
		const ArchetypeComponents& archetypeComponents = hierarchy.GetCompatibleArchetypes().GetArchetypeComponents(node.Archetype);

		size_t offset = 0;
		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& info = hierarchy.GetCompatibleComponents().GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
			size_t componentOffset = ComputeNextComponentOffset(offset, info);

			YAMLSerializer serializer(emitter, nullptr);
			emitter << YAML::BeginMap;
			emitter << YAML::Key << "Name" << YAML::Value << info.Initializer->SerializationDescriptor.Name;

			emitter << YAML::Key << "Data" << YAML::Value;
			serializer.SerializeObject(info.Initializer->SerializationDescriptor, entityData + componentOffset, false, 0);
			emitter << YAML::EndMap;
		}

	}

	static void SerializePrefabHierarchy(YAML::Emitter& emitter, const PrefabHierarchy& hierarchy)
	{
		FLARE_PROFILE_FUNCTION();

		emitter << YAML::BeginSeq;

		const auto& nodes = hierarchy.GetNodes();
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			emitter << YAML::BeginMap;

			emitter << YAML::Key << "Parent" << YAML::Value;
			if (nodes[nodeIndex].ParentNode == PrefabHierarchy::Node::INVALID_PARENT_NODE)
				emitter << YAML::Null;
			else
				emitter << nodes[nodeIndex].ParentNode;

			emitter << YAML::Key << "Components" << YAML::Value << YAML::BeginSeq;
			SerializeNodeComponents(emitter, hierarchy, nodeIndex);
			emitter << YAML::EndSeq;

			emitter << YAML::EndMap;
		}

		emitter << YAML::EndSeq;
	}

	static void SerializePrefabFlags(YAML::Emitter& emitter, PrefabFlags flags)
	{
		FLARE_PROFILE_FUNCTION();
		if (flags == PrefabFlags::None)
			return;

		if (HAS_BIT(flags, PrefabFlags::Generated))
			emitter << YAML::Value << "Generated";
	}

	void PrefabImporter::SerializePrefab(AssetHandle prefab)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(prefab));

		YAML::Emitter emitter;
		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(prefab);
		Ref<Prefab> prefabAsset = AssetManager::GetAsset<Prefab>(prefab);

		emitter << YAML::BeginMap;

		emitter << YAML::Key << "Flags" << YAML::Value << YAML::BeginSeq;
		SerializePrefabFlags(emitter, prefabAsset->GetFlags());
		emitter << YAML::EndSeq;

		emitter << YAML::Key << "Hierarchy" << YAML::Value;

		if (!HAS_BIT(prefabAsset->GetFlags(), PrefabFlags::Generated))
			SerializePrefabHierarchy(emitter, prefabAsset->GetHierarchy());
		else
			emitter << YAML::Null;

		emitter << YAML::EndMap;

		std::ofstream output(metadata->Path);
		output << emitter.c_str();
		output.close();
	}

	void PrefabImporter::SerializeGeneratedPrefab(Ref<Prefab> prefab, AssetHandle generatorHandle)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(prefab->Handle));
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(generatorHandle));

		FLARE_CORE_ASSERT(HAS_BIT(prefab->GetFlags(), PrefabFlags::Generated));

		YAML::Emitter emitter;

		emitter << YAML::BeginMap;

		emitter << YAML::Key << "Flags" << YAML::Value << YAML::BeginSeq;
		SerializePrefabFlags(emitter, prefab->GetFlags());
		emitter << YAML::EndSeq;

		emitter << YAML::Key << "SourceAsset" << YAML::Value << generatorHandle;

		emitter << YAML::EndMap;

		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(prefab->Handle);

		std::ofstream file(metadata->Path);
		file << emitter.c_str();
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

			size_t parent = PrefabHierarchy::Node::INVALID_PARENT_NODE;
			if (YAML::Node parentNode = entityNode["Parent"])
			{
				if (!parentNode.IsNull())
					parent = parentNode.as<size_t>();
			}

			for (YAML::Node componentNode : components)
			{
				if (YAML::Node name = componentNode["Name"])
				{
					std::string componentName = name.as<std::string>();
					std::optional<ComponentId> componentId = componentsRegistry.FindComponent(componentName);

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

			hierarchy.AddEntity(archetype->Id, parent);
		}

		hierarchy.EnsureAllocated();

		for (size_t nodeIndex = 0; nodeIndex < dataNodes.size(); nodeIndex++)
		{
			const auto& node = dataNodes[nodeIndex];
			uint8_t* entityData = hierarchy.GetEntityData(nodeIndex);

			const ArchetypeComponents& archetypeComponents = hierarchy.GetCompatibleArchetypes().GetArchetypeComponents(node.Archetype);

			size_t offset = 0;
			for (size_t componentIndex = 0; componentIndex < node.ComponentIds.size(); componentIndex++)
			{
				const ComponentInfo* component = node.ComponentIds[componentIndex];
				size_t componentOffset = ComputeNextComponentOffset(offset, *component);

				EntitySizeT archetypeComponentIndex = archetypeComponents.TryGetComponentIndex(component->Id);
				FLARE_CORE_ASSERT(archetypeComponentIndex != ArchetypeComponents::INVALID_COMPONENT_INDEX);

				uint8_t* componentData = entityData + componentOffset;

				component->Initializer->Type.Functions.DefaultConstructor(componentData);

				YAMLDeserializer deserializer(node.ComponentNodes[componentIndex], nullptr);
				deserializer.PropertyKey("Data");
				deserializer.SerializeObject(component->Initializer->SerializationDescriptor, componentData, false, 0);
			}
		}
	}

	Ref<Asset> PrefabImporter::ImportPrefab(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();

		ECSContext& context = EditorLayer::GetInstance().GetECSContext();

		YAML::Node node;
		try
		{
			node = YAML::LoadFile(metadata.Path.generic_string());
		}
		catch (std::exception& e)
		{
			FLARE_CORE_ERROR("Failed to import prefab {}. Error: {}", (uint64_t)metadata.Handle, e.what());
			return nullptr;
		}

		PrefabFlags flags = PrefabFlags::None;
		if (YAML::Node flagsNode = node["Flags"])
		{
			for (YAML::Node flagNode : flagsNode)
			{
				std::string flagString = flagNode.as<std::string>();
				if (flagString == "Generated")
					flags |= PrefabFlags::Generated;
			}
		}

		if (HAS_BIT(flags, PrefabFlags::Generated))
		{
			AssetHandle sourceAsset = NULL_ASSET_HANDLE;
			if (YAML::Node sourceAssetNode = node["SourceAsset"])
				sourceAsset = sourceAssetNode.as<AssetHandle>();

			if (sourceAsset == NULL_ASSET_HANDLE)
			{
				FLARE_CORE_ERROR("Failed to import prefab '{}' because it has a 'Generated' flag, but 'SourceAsset' is null", metadata.Name);
				return nullptr;
			}

			if (!AssetManager::IsAssetHandleValid(sourceAsset))
			{
				FLARE_CORE_ERROR("Failed to import prefab '{}' because it has a 'Generated' flag but 'SourceAsset' is invalid", metadata.Name);
				return nullptr;
			}

			return MeshImporter::ImportAsPrefab(*AssetManager::GetAssetMetadata(sourceAsset));
		}

		Ref<Prefab> prefab = Ref<Prefab>::New(context.Components, context.Archetypes, PrefabFlags::None);
		if (YAML::Node hierarchyNode = node["Hierarchy"])
			DeserializePrefabHierarchy(hierarchyNode, prefab->GetHierarchy());

		return prefab;
	}
}
