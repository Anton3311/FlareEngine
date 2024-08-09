#include "Prefab.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
	//
	// PrefabHierarchy
	//

	PrefabHierarchy::PrefabHierarchy(const Components& compatibleComponents, const Archetypes& compatibleArchetypes)
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

		// TODO: Patch entity references, because the ones copyed are only valid inside the given world
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

	Prefab::Prefab(const uint8_t* prefabData,
		const Components& compatibleComponentsRegistry,
		const Archetypes& compatibleArchetypes,
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
		return world.Entities.CreateEntity(m_Components.data(), m_Components.size(), true);
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
