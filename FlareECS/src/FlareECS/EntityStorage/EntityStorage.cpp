#include "EntityStorage.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentInitializer.h"

#include <cmath>

namespace Flare
{
	size_t EntityStorage::AddEntity(Entity entity)
	{
		if (m_EntityCount % m_EntitiesPerChunk == 0)
			m_Chunks.push_back(EntityChunksPool::GetInstance()->GetOrCreate());

		m_EntityCount++;
		size_t entityIndex = m_EntityCount - 1;

		UpdateEntityIdEntry(m_Chunks.size() - 1, entity, entityIndex % m_EntitiesPerChunk);

		return entityIndex;
	}

	void EntityStorage::RemoveEntity(size_t index)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(index < m_EntityCount);

		if (index != m_EntityCount - 1)
		{
			const ArchetypeRecord& archetype = m_ArchetypesRegistry->operator[](m_Archetype);
			const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
			for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
			{
				const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents().GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

				void* destination = GetEntityComponentData(index, componentIndex);
				const void* source = GetEntityComponentData(m_EntityCount - 1, componentIndex);

				componentInfo.Initializer->Type.Functions.CopyAssignment(destination, source);
			}

			Entity lastEntityId = GetEntityId(m_EntityCount - 1);

			size_t chunkIndex = index / m_EntitiesPerChunk;
			size_t indexInChunk = index % m_EntitiesPerChunk;

			UpdateEntityIdEntry(chunkIndex, lastEntityId, indexInChunk);
			InvalidateEntityIdEntry(m_Chunks.size() - 1, (m_EntityCount - 1) % m_EntitiesPerChunk);
		}

		// Destroy the last entity, because
		// 1. The caller asked to remove this entity
		// 2. The caller asked to remove other entity, and last entity was moved in place of the removed one.
		//
		// In both cases removing an entity requires popping the last one
		ReleaseEntityComponents(m_EntityCount - 1);

		m_EntityCount--;

		if (m_EntityCount % m_EntitiesPerChunk == 0)
		{
			EntityChunksPool::GetInstance()->Add(std::move(m_Chunks.back()));
			m_Chunks.erase(m_Chunks.end() - 1);
		}
	}

	size_t EntityStorage::GetEntitiesCountInChunk(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_Chunks.size());

		if (index == m_Chunks.size() - 1)
			return m_EntityCount - (index * m_EntitiesPerChunk);
		return m_EntitiesPerChunk;
	}

	void EntityStorage::Initialize(const EntityStorageRequirements& storageRequirements, const Archetypes& archetypes, ArchetypeId archetype)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(archetypes.IsIdValid(archetype));
		
		m_ArchetypesRegistry = &archetypes;
		m_Archetype = archetype;

		FLARE_CORE_ASSERT(storageRequirements.IsValid());
		m_StorageRequirements = storageRequirements;

		m_EntitiesPerChunk = archetypes[archetype].EntityCountPerChunk;
	}

	void EntityStorage::Release()
	{
		FLARE_PROFILE_FUNCTION();

		ReleaseAllEntities();

		m_StorageRequirements = {};
	}

	void* EntityStorage::GetEntityComponentData(size_t entityIndex, size_t componentIndex) const
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(entityIndex < m_EntityCount);

		size_t chunkIndex = entityIndex / m_EntitiesPerChunk;
		size_t indexInChunk = entityIndex % m_EntitiesPerChunk;

		FLARE_CORE_ASSERT(indexInChunk < GetEntitiesCountInChunk(chunkIndex));

		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);

		uint8_t* componentArray = (uint8_t*)GetComponentArray(chunkIndex, componentIndex);
		size_t componentOffset = indexInChunk * m_ArchetypesRegistry->GetCompatibleComponents()
			.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]).Size;

		return componentArray + componentOffset;
	}

	void EntityStorage::DefaultConstructEntityComponentsRange(size_t entityIndex, size_t startComponent, size_t componentCount)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(entityIndex < m_EntityCount);

		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
		FLARE_CORE_ASSERT(startComponent + componentCount <= archetypeComponents.ComponentCount);

		size_t endComponent = startComponent + componentCount;

		for (size_t componentIndex = startComponent; componentIndex < endComponent; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

			componentInfo.Initializer->Type.Functions.DefaultConstructor(GetEntityComponentData(entityIndex, componentIndex));
		}
	}

	void EntityStorage::CopyConstructEntity(size_t entityIndex, Span<const void*> componentData)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(entityIndex < m_EntityCount);

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
		FLARE_CORE_ASSERT(componentData.GetSize() == archetypeComponents.ComponentCount);

		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

			componentInfo.Initializer->Type.Functions.CopyConstructor(GetEntityComponentData(entityIndex, componentIndex), componentData[componentIndex]);
		}
	}

	void EntityStorage::MoveConstructEntity(size_t entityIndex, Span<void*> componentData)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(entityIndex < m_EntityCount);

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
		FLARE_CORE_ASSERT(componentData.GetSize() == archetypeComponents.ComponentCount);

		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

			componentInfo.Initializer->Type.Functions.MoveConstructor(GetEntityComponentData(entityIndex, componentIndex), componentData[componentIndex]);
		}
	}

	void EntityStorage::MoveEntityData(size_t sourceEntityIndex, EntityStorage& sourceStorage, size_t destinationEntityIndex)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(sourceEntityIndex < sourceStorage.GetEntityCount() && destinationEntityIndex < m_EntityCount);
		FLARE_CORE_ASSERT(m_ArchetypesRegistry == sourceStorage.m_ArchetypesRegistry && m_Archetype == sourceStorage.m_Archetype);

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

			componentInfo.Initializer->Type.Functions.MoveAssignment(
				GetEntityComponentData(destinationEntityIndex, componentIndex),
				GetEntityComponentData(sourceEntityIndex, componentIndex));
		}
	}

	void EntityStorage::ReleaseEntityComponents(size_t entityIndex)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(entityIndex < m_EntityCount);

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

			componentInfo.Initializer->Type.Functions.Destructor(GetEntityComponentData(entityIndex, componentIndex));
		}
	}

	void EntityStorage::ReleaseAllEntities()
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
		for (size_t componentIndex = 0; componentIndex < archetypeComponents.ComponentCount; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeComponents.ComponentIds[componentIndex]);

			for (size_t entityIndex = 0; entityIndex < m_EntityCount; entityIndex++)
			{
				void* componentData = GetEntityComponentData(entityIndex, componentIndex);
				componentInfo.Initializer->Type.Functions.Destructor(componentData);
			}
		}

		for (EntityStorageChunk& chunk : m_Chunks)
			EntityChunksPool::GetInstance()->Add(std::move(chunk));

		m_Chunks.clear();
		m_EntityCount = 0;
	}

	Entity EntityStorage::GetEntityId(size_t entityIndex) const
	{
		FLARE_CORE_ASSERT(entityIndex < m_EntityCount);

		size_t chunkIndex = entityIndex / m_EntitiesPerChunk;
		size_t indexInChunk = (entityIndex % m_EntitiesPerChunk);
	
		return ReadEntityIdEntry(chunkIndex, indexInChunk);
	}
}