#include "Prefab.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

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

		for (size_t entityIndex = 0; entityIndex < m_Nodes.size(); entityIndex++)
		{
			Node& node = m_Nodes[entityIndex];
			node.Archetype = entities[entityIndex].Archetype;
			node.DataOffset = m_BufferSize; // TODO: Consider alignment?
			node.Size = m_CompatibleArchetypes[node.Archetype].EntitySize;

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
				componentInfo.Initializer->Type.DefaultConstructor(componentData);
				componentInfo.Initializer->Type.CopyConstructor(componentData, sceneEntityComponent);
			}
		}

		// TODO: Patch entity references, because the ones copied are only valid inside the given world
	}

	void PrefabHierarchy::AddEntity(ArchetypeId archetype)
	{
		FLARE_CORE_ASSERT(m_CompatibleArchetypes.IsIdValid(archetype));
		FLARE_CORE_ASSERT(IsEmpty());

		const ArchetypeRecord& record = m_CompatibleArchetypes[archetype];

		size_t offset = 0;
		if (m_Nodes.size() > 0)
			offset = m_Nodes.back().DataOffset + m_Nodes.back().Size;

		Node& node = m_Nodes.emplace_back();
		node.Archetype = archetype;
		node.DataOffset = offset;
		node.Size = record.EntitySize;
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
				componentInfo.Initializer->Type.Destructor(m_Buffer + node.DataOffset + archetypeRecord.ComponentOffsets[componentIndex]);
			}
		}

		std::memset(m_Buffer, 0, m_BufferSize);
	}

	//
	// Prefab
	//

	FLARE_IMPL_ASSET(Prefab);
	FLARE_SERIALIZABLE_IMPL(Prefab);

	Prefab::Prefab(const Components& compatibleComponentsRegistry, Archetypes& compatibleArchetypes)
		: Asset(AssetType::Prefab),
		m_CompatibleComponentsRegistry(compatibleComponentsRegistry),
		m_CompatibleArchetypes(compatibleArchetypes),
		m_Hierarchy(compatibleComponentsRegistry, compatibleArchetypes)	{}

	Prefab::Prefab(const uint8_t* prefabData,
		const Components& compatibleComponentsRegistry,
		Archetypes& compatibleArchetypes,
		std::vector<std::pair<ComponentId, void*>>&& components)
		: Asset(AssetType::Prefab),
		m_Data(prefabData),
		m_Hierarchy(compatibleComponentsRegistry, compatibleArchetypes),
		m_Components(std::move(components)),
		m_CompatibleComponentsRegistry(compatibleComponentsRegistry),
		m_CompatibleArchetypes(compatibleArchetypes)
	{
	}

	Prefab::~Prefab()
	{
		if (m_Data != nullptr)
		{
			for (const auto& [id, data] : m_Components)
			{
				auto& info = m_CompatibleComponentsRegistry.GetComponentInfo(id);
				info.Deleter(data);
			}

			delete[] m_Data;
		}
	}

	Entity Prefab::CreateInstance(World& world)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(&world.Components == &m_CompatibleComponentsRegistry);
#if 0
		return world.Entities.CreateEntity(m_Components.data(), m_Components.size(), true);
#else
		return IntantiateHierarchy(world);
#endif
	}

	Entity Prefab::IntantiateHierarchy(World& world) const
	{
		FLARE_PROFILE_FUNCTION();

		const auto& nodes = m_Hierarchy.GetNodes();
		FLARE_CORE_ASSERT(nodes.size() > 0);

		Entity rootEntity = Entity();

		for (size_t i = 0; i < nodes.size(); i++)
		{
			const auto& node = nodes[i];

			Entity entity = world.Entities.CreateEntityFromArchetype(node.Archetype, ComponentInitializationStrategy::DefaultConstructor);
			if (i == 0)
				rootEntity = entity;

			const uint8_t* hierarchyEntityData = m_Hierarchy.GetEntityData(i);

			std::optional<uint8_t*> worldEntityData = world.Entities.GetEntityData(entity);
			FLARE_CORE_ASSERT(worldEntityData);

			const ArchetypeRecord& archetype = m_Hierarchy.GetCompatibleArchetypes()[node.Archetype];
			for (size_t componentIndex = 0; componentIndex < archetype.Components.size(); componentIndex++)
			{
				const ComponentInfo& component = m_Hierarchy.GetCompatibleComponents().GetComponentInfo(archetype.Components[componentIndex]);
				size_t componentOffset = archetype.ComponentOffsets[componentIndex];
				uint8_t* componentData = *worldEntityData + componentOffset;

				component.Initializer->Type.CopyConstructor(componentData, hierarchyEntityData + componentOffset);
			}
		}

		return rootEntity;
	}

	InstantiatePrefab::InstantiatePrefab(const Ref<Prefab>& prefab)
		: m_Prefab(prefab) {}

	//
	// InstantiatePrefab
	//

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
