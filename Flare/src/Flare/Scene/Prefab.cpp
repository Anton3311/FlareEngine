#include "PCH.h"

#include "Prefab.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Hierarchy.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
	//
	// PrefabHierarchy
	//

	PrefabHierarchy::PrefabHierarchy(const Components& compatibleComponents, Archetypes& compatibleArchetypes)
		: m_CompatibleComponentsRegistry(compatibleComponents), m_CompatibleArchetypes(compatibleArchetypes)
	{
	}

	PrefabHierarchy::~PrefabHierarchy()
	{
		FLARE_PROFILE_FUNCTION();
		Release();
	}

	void PrefabHierarchy::CopyFromWorld(const World& world)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(&world.Components == &m_CompatibleComponentsRegistry);
		FLARE_CORE_ASSERT(&world.GetArchetypes() == &m_CompatibleArchetypes);

		Release();

		const std::vector<EntityRecord>& entities = world.Entities.GetEntityRecords();
		if (entities.size() == 0)
			return;

		m_Nodes.resize(entities.size());

		std::unordered_map<Entity, size_t> entityToNode;

		for (size_t entityIndex = 0; entityIndex < m_Nodes.size(); entityIndex++)
		{
			Node& node = m_Nodes[entityIndex];
			node.Archetype = entities[entityIndex].Archetype;
			node.DataOffset = m_BufferSize; // TODO: Consider alignment?
			node.Size = m_CompatibleArchetypes[node.Archetype].EntitySize;

			entityToNode[entities[entityIndex].Id] = entityIndex;

			m_BufferSize += node.Size;
		}

		FLARE_CORE_ASSERT(m_Buffer == nullptr);
		m_Buffer = new uint8_t[m_BufferSize];

		for (size_t entityIndex = 0; entityIndex < m_Nodes.size(); entityIndex++)
		{
			const Node& node = m_Nodes[entityIndex];
			const ArchetypeRecord& archetypeRecord = m_CompatibleArchetypes[node.Archetype];

			for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
			{
				uint8_t* componentData = m_Buffer + node.DataOffset + archetypeRecord.ComponentOffsets[componentIndex];
				const void* sceneEntityComponent = world.Entities.GetEntityComponent(
					entities[entityIndex].Id,
					archetypeRecord.Components[componentIndex]);

				FLARE_CORE_ASSERT(sceneEntityComponent);

				const ComponentInfo& componentInfo = m_CompatibleComponentsRegistry.GetComponentInfo(archetypeRecord.Components[componentIndex]);
				componentInfo.Initializer->Type.Functions.CopyConstructor(componentData, sceneEntityComponent);
			}
		}

		// NOTE: Parent component is hardcoded, any other component that contains a reference to an entity will not be serialized
		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.size(); nodeIndex++)
		{
			const EntityRecord& entityRecord = entities[nodeIndex];

			const Parent* parent = world.TryGetEntityComponent<const Parent>(entityRecord.Id);
			if (parent && world.IsEntityAlive(parent->ParentEntity))
			{
				auto it = entityToNode.find(parent->ParentEntity);
				if (it == entityToNode.end())
				{
					FLARE_CORE_WARN("PrefabHierarchy: Entity {} has invalid parent entity {}", entityRecord.Id.GetIndex(), parent->ParentEntity.GetIndex());
					continue;
				}
				
				m_Nodes[nodeIndex].ParentNode = it->second;
			}
		}
	}

	void PrefabHierarchy::AddEntity(ArchetypeId archetype, size_t parentIndex)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_CompatibleArchetypes.IsIdValid(archetype));
		FLARE_CORE_ASSERT(IsEmpty());

		const ArchetypeRecord& record = m_CompatibleArchetypes[archetype];

		size_t offset = 0;
		if (m_Nodes.size() == 0)
		{
			FLARE_CORE_ASSERT(parentIndex == Node::INVALID_PARENT_NODE, "Root node is not allowed to have a parent node");
		}
		else
		{
			FLARE_CORE_ASSERT(parentIndex != Node::INVALID_PARENT_NODE, "All nodes (except the root) must have a valid parent node");
			FLARE_CORE_ASSERT(parentIndex < m_Nodes.size());
			offset = m_Nodes.back().DataOffset + m_Nodes.back().Size;
		}

		Node& node = m_Nodes.emplace_back();
		node.Archetype = archetype;
		node.DataOffset = offset;
		node.Size = record.EntitySize;
		node.ParentNode = parentIndex;
	}

	uint8_t* PrefabHierarchy::GetEntityData(size_t nodeIndex) const
	{
		FLARE_CORE_ASSERT(nodeIndex < m_Nodes.size());
		FLARE_CORE_ASSERT(m_Buffer);

		const Node& node = m_Nodes[nodeIndex];

		FLARE_CORE_ASSERT(node.DataOffset + node.Size <= m_BufferSize);
		return m_Buffer + node.DataOffset;
	}

	void PrefabHierarchy::EnsureAllocated()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Nodes.size() == 0)
			return;

		const Node& lastNode = m_Nodes.back();
		
		m_BufferSize = lastNode.DataOffset + lastNode.Size;
		m_Buffer = new uint8_t[m_BufferSize];
	}

	void PrefabHierarchy::InitializeEntities()
	{
		FLARE_PROFILE_FUNCTION();

		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.size(); nodeIndex++)
		{
			EntityHelper::DefaultConstruct(m_CompatibleArchetypes[m_Nodes[nodeIndex].Archetype], m_CompatibleComponentsRegistry, GetEntityData(nodeIndex));
		}
	}

	std::optional<size_t> PrefabHierarchy::GetNodeComponentOffset(size_t nodeIndex, ComponentId component) const
	{
		const auto& node = m_Nodes[nodeIndex];

		const ArchetypeRecord& archetype = m_CompatibleArchetypes[node.Archetype];
		std::optional<size_t> componentIndex = archetype.TryGetComponentIndex(component);

		if (!componentIndex)
			return {};

		return archetype.ComponentOffsets[*componentIndex];
	}

	void PrefabHierarchy::Release()
	{
		FLARE_PROFILE_FUNCTION();
		ReleaseEntityData();

		if (m_Buffer)
			delete[] m_Buffer;

		m_Buffer = nullptr;
		m_BufferSize = 0;
	}

	void PrefabHierarchy::ReleaseEntityData()
	{
		FLARE_PROFILE_FUNCTION();
		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.size(); nodeIndex++)
		{
			const Node& node = m_Nodes[nodeIndex];

			FLARE_CORE_ASSERT(node.Archetype != INVALID_ARCHETYPE_ID);
			const ArchetypeRecord& archetypeRecord = m_CompatibleArchetypes[node.Archetype];

			for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
			{
				const ComponentInfo& componentInfo = m_CompatibleComponentsRegistry.GetComponentInfo(archetypeRecord.Components[componentIndex]);
				componentInfo.Initializer->Type.Functions.Destructor(m_Buffer + node.DataOffset + archetypeRecord.ComponentOffsets[componentIndex]);
			}
		}

		std::memset(m_Buffer, 0, m_BufferSize);
	}

	//
	// Prefab
	//

	FLARE_IMPL_ASSET(Prefab);
	FLARE_SERIALIZABLE_IMPL(Prefab);

	Prefab::Prefab(const Components& compatibleComponentsRegistry, Archetypes& compatibleArchetypes, PrefabFlags flags)
		: Asset(AssetType::Prefab),
		m_CompatibleComponentsRegistry(compatibleComponentsRegistry),
		m_CompatibleArchetypes(compatibleArchetypes),
		m_Hierarchy(compatibleComponentsRegistry, compatibleArchetypes),
		m_Flags(flags) {}

	Prefab::Prefab(const Components& compatibleComponentsRegistry, Archetypes& compatibleArchetypes, PrefabFlags flags, AssetHandle sourceMesh)
		: Asset(AssetType::Prefab),
		m_CompatibleComponentsRegistry(compatibleComponentsRegistry),
		m_CompatibleArchetypes(compatibleArchetypes),
		m_Hierarchy(compatibleComponentsRegistry, compatibleArchetypes),
		m_Flags(flags), m_SourceMesh(sourceMesh) {}

	Entity Prefab::CreateInstance(World& world)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(&world.Components == &m_CompatibleComponentsRegistry);
		return InstantiateHierarchy(world);
	}

	Entity Prefab::InstantiateHierarchy(World& world) const
	{
		FLARE_PROFILE_FUNCTION();

		const auto& nodes = m_Hierarchy.GetNodes();
		FLARE_CORE_ASSERT(nodes.size() > 0);

		std::vector<Entity> createdEntities(m_Hierarchy.GetNodes().size(), Entity());

		for (size_t i = 0; i < nodes.size(); i++)
		{
			const auto& node = nodes[i];
			bool isRoot = i == 0;

			Entity entity = world.Entities.CreateEntityFromArchetype(node.Archetype, ComponentInitializationStrategy::NoInitialization);
			createdEntities[i] = entity;

			const uint8_t* hierarchyEntityData = m_Hierarchy.GetEntityData(i);
			const ArchetypeRecord& archetype = m_Hierarchy.GetCompatibleArchetypes()[node.Archetype];
			for (size_t componentIndex = 0; componentIndex < archetype.Components.size(); componentIndex++)
			{
				const ComponentInfo& component = m_Hierarchy.GetCompatibleComponents().GetComponentInfo(archetype.Components[componentIndex]);
				size_t componentOffset = archetype.ComponentOffsets[componentIndex];
				uint8_t* componentData = (uint8_t*)world.Entities.GetEntityComponent(entity, component.Id);

				if (component.Id == COMPONENT_ID(Children))
				{
					component.Initializer->Type.Functions.DefaultConstructor(componentData);
				}
				else
				{
					component.Initializer->Type.Functions.CopyConstructor(componentData, hierarchyEntityData + componentOffset);
				}
			}

			if (!isRoot)
			{
				Parent* parent = world.TryGetEntityComponent<Parent>(entity);
				FLARE_CORE_ASSERT(parent, "An entity that is not at the root of PrefabHierarchy must have a Parent component");

				FLARE_CORE_ASSERT(node.ParentNode != PrefabHierarchy::Node::INVALID_PARENT_NODE);

				parent->ParentEntity = createdEntities[node.ParentNode];

				Children* children = world.TryGetEntityComponent<Children>(parent->ParentEntity);
				FLARE_CORE_ASSERT(children);

				children->ChildrenEntities.push_back(createdEntities[i]);
			}
		}

		FLARE_CORE_ASSERT(createdEntities.size() > 0);
		return createdEntities[0];
	}

	//
	// InstantiatePrefab
	//

	InstantiatePrefab::InstantiatePrefab(const Ref<Prefab>& prefab)
		: m_Prefab(prefab) {}

	void InstantiatePrefab::Apply(CommandContext& context, World& world)
	{
		FLARE_PROFILE_FUNCTION();
		Entity entity = m_Prefab->CreateInstance(world);
		context.SetEntity(m_OutputEntity, entity);
	}

	void InstantiatePrefab::Initialize(FutureEntity entity)
	{
		m_OutputEntity = entity;
	}
}
