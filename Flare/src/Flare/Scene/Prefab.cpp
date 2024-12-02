#include "PCH.h"

#include "Prefab.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Hierarchy.h"
#include "Flare/Scene/Transform.h"

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
			const ArchetypeComponents& archetypeComponents = m_CompatibleArchetypes.GetArchetypeComponents(node.Archetype);

			for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
			{
				uint8_t* componentData = m_Buffer + node.DataOffset + archetypeRecord.ComponentOffsets[componentIndex];
				const void* sceneEntityComponent = world.Entities.GetEntityComponent(
					entities[entityIndex].Id,
					archetypeComponents.ComponentIds[componentIndex]);

				FLARE_CORE_ASSERT(sceneEntityComponent);

				const ComponentInfo& componentInfo = m_CompatibleComponentsRegistry.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
				componentInfo.Initializer->Type.Functions.CopyConstructor(componentData, sceneEntityComponent);
			}
		}

		// NOTE: Parent component is hardcoded, any other component that contains a reference to an entity will not be serialized
		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.size(); nodeIndex++)
		{
			const EntityRecord& entityRecord = entities[nodeIndex];

			const Parent* parent = world.TryGetEntityComponent<const Parent>(entityRecord.Id);
			if (parent && world.IsEntityAlive(parent->GetParentEntity()))
			{
				auto it = entityToNode.find(parent->GetParentEntity());
				if (it == entityToNode.end())
				{
					FLARE_CORE_WARN("PrefabHierarchy: Entity {} has invalid parent entity {}", entityRecord.Id.GetIndex(), parent->GetParentEntity().GetIndex());
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
			EntityHelper::DefaultConstruct(m_CompatibleArchetypes, m_Nodes[nodeIndex].Archetype, m_CompatibleComponentsRegistry, GetEntityData(nodeIndex));
		}
	}

	std::optional<size_t> PrefabHierarchy::GetNodeComponentOffset(size_t nodeIndex, ComponentId component) const
	{
		FLARE_PROFILE_FUNCTION();
		const auto& node = m_Nodes[nodeIndex];

		const ArchetypeRecord& archetype = m_CompatibleArchetypes[node.Archetype];
		const ArchetypeComponents& archetypeComponents = m_CompatibleArchetypes.GetArchetypeComponents(node.Archetype);

		EntitySizeT componentIndex = archetypeComponents.TryGetComponentIndex(component);

		if (componentIndex == ArchetypeComponents::INVALID_COMPONENT_INDEX)
			return {};

		return archetype.ComponentOffsets[componentIndex];
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
			const ArchetypeComponents& archetypeComponents = m_CompatibleArchetypes.GetArchetypeComponents(node.Archetype);

			for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
			{
				const ComponentInfo& componentInfo = m_CompatibleComponentsRegistry.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
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

	std::optional<Entity> Prefab::TryCreateInstance(World& world)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(&world.Components == &m_CompatibleComponentsRegistry);

		if (m_Hierarchy.IsEmpty())
			return {};

		return InstantiateHierarchy(world);
	}

	Ref<Prefab> Prefab::CreateEmpty(Archetypes& compatibleArchetypes)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Prefab> prefab = Ref<Prefab>::New(compatibleArchetypes.GetCompatibleComponents(), compatibleArchetypes, PrefabFlags::None);

		ComponentId components[] = { COMPONENT_ID(TransformComponent) };
		ArchetypeId archetype = prefab->GetHierarchy().GetCompatibleArchetypes().FindOrCreateArchetype(Span(components, 1))->Id;

		prefab->GetHierarchy().AddEntity(archetype, PrefabHierarchy::Node::INVALID_PARENT_NODE);
		prefab->GetHierarchy().EnsureAllocated();
		prefab->GetHierarchy().InitializeEntities();

		return prefab;
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
			const ArchetypeComponents& archetypeComponents = m_Hierarchy.GetCompatibleArchetypes().GetArchetypeComponents(node.Archetype);
			for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
			{
				const ComponentInfo& component = m_Hierarchy.GetCompatibleComponents().GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);
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
				FLARE_CORE_ASSERT(node.ParentNode != PrefabHierarchy::Node::INVALID_PARENT_NODE);

				HierarchyHelper::SetParent(world, createdEntities[i], createdEntities[node.ParentNode]);
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
