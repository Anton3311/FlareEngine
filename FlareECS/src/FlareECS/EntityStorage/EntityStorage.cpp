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

		UpdateEntityIdEntry(m_Chunks.back(), entity, entityIndex % m_EntitiesPerChunk);

		return entityIndex;
	}

	void EntityStorage::RemoveEntity(size_t index)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(index < m_EntityCount);

		if (index != m_EntityCount - 1)
		{
			const ArchetypeRecord& archetype = m_ArchetypesRegistry->operator[](m_Archetype);
			for (size_t componentIndex = 0; componentIndex < archetype.Components.size(); componentIndex++)
			{
				const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents().GetComponentInfo(archetype.Components[componentIndex]);

				void* destination = GetEntityComponentData(index, componentIndex);
				const void* source = GetEntityComponentData(m_EntityCount - 1, componentIndex);

				componentInfo.Initializer->Type.Functions.CopyAssignment(destination, source);
			}

			Entity lastEntityId = GetEntityId(m_EntityCount - 1);

			size_t chunkIndex = index / m_EntitiesPerChunk;
			size_t indexInChunk = index % m_EntitiesPerChunk;

			UpdateEntityIdEntry(m_Chunks[chunkIndex], lastEntityId, indexInChunk);
			InvalidateEntityIdEntry(m_Chunks.back(), (m_EntityCount - 1) % m_EntitiesPerChunk);
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
			return m_EntityCount % m_EntitiesPerChunk;
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

		// TODO: Account for 2 byte alignment of packed entity id
		size_t entityAndIdSize = m_StorageRequirements.EntitySize + PACKED_ENTITY_ID_SIZE;

		m_EntitiesPerChunk = (size_t)floor((float)EntityStorageChunk::CHUNK_SIZE / (float)entityAndIdSize);
		m_Layout.ChunkSize = m_EntitiesPerChunk * m_StorageRequirements.EntitySize;
		m_Layout.EntityStride = m_StorageRequirements.EntitySize;
		m_Layout.IdOffset = m_Layout.ChunkSize;
		m_Layout.IdStride = PACKED_ENTITY_ID_SIZE;
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

		size_t chunkIndex = entityIndex / m_EntitiesPerChunk;
		size_t indexInChunk = entityIndex % m_EntitiesPerChunk;

		const ArchetypeRecord& archetype = m_ArchetypesRegistry->operator[](m_Archetype);

		uint8_t* componentArray = (uint8_t*)GetComponentArray(chunkIndex, componentIndex);
		size_t componentOffset = entityIndex * m_ArchetypesRegistry->GetCompatibleComponents()
			.GetComponentInfo(archetype.Components[componentIndex]).Size;

		return componentArray + componentOffset;
	}

	void EntityStorage::DefaultConstructEntityComponentsRange(size_t entityIndex, size_t startComponent, size_t componentCount)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		FLARE_CORE_ASSERT(startComponent + componentCount <= archetypeRecord.Components.size());

		size_t endComponent = startComponent + componentCount;

		for (size_t componentIndex = startComponent; componentIndex < endComponent; componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeRecord.Components[componentIndex]);

			componentInfo.Initializer->Type.Functions.DefaultConstructor(GetEntityComponentData(entityIndex, componentIndex));
		}
	}

	void EntityStorage::CopyConstructEntity(size_t entityIndex, Span<const void*> componentData)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		FLARE_CORE_ASSERT(componentData.GetSize() == archetypeRecord.Components.size());

		for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeRecord.Components[componentIndex]);

			componentInfo.Initializer->Type.Functions.CopyConstructor(GetEntityComponentData(entityIndex, componentIndex), componentData[componentIndex]);
		}
	}

	void EntityStorage::MoveConstructEntity(size_t entityIndex, Span<void*> componentData)
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		FLARE_CORE_ASSERT(componentData.GetSize() == archetypeRecord.Components.size());

		for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeRecord.Components[componentIndex]);

			componentInfo.Initializer->Type.Functions.MoveConstructor(GetEntityComponentData(entityIndex, componentIndex), componentData[componentIndex]);
		}
	}

	void EntityStorage::MoveEntityData(size_t sourceEntityIndex, EntityStorage& sourceStorage, size_t destinationEntityIndex)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_ArchetypesRegistry == sourceStorage.m_ArchetypesRegistry && m_Archetype == sourceStorage.m_Archetype);

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeRecord.Components[componentIndex]);

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
		for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeRecord.Components[componentIndex]);

			componentInfo.Initializer->Type.Functions.Destructor(GetEntityComponentData(entityIndex, componentIndex));
		}
	}

	void EntityStorage::ReleaseAllEntities()
	{
		FLARE_PROFILE_FUNCTION();

		const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
		for (size_t componentIndex = 0; componentIndex < archetypeRecord.Components.size(); componentIndex++)
		{
			const ComponentInfo& componentInfo = m_ArchetypesRegistry->GetCompatibleComponents()
				.GetComponentInfo(archetypeRecord.Components[componentIndex]);

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
	
		return ReadEntityIdEntry(m_Chunks[chunkIndex], indexInChunk);
	}

	void EntityStorage::InvalidateEntityIdEntry(EntityStorageChunk& chunk, size_t entityIndexInChunk)
	{
		std::memset(chunk.GetBuffer() + GetIdEntryOffset(entityIndexInChunk), 0xff, PACKED_ENTITY_ID_SIZE);
	}

	void EntityStorage::UpdateEntityIdEntry(EntityStorageChunk& chunk, Entity entityId, size_t entityIndexInChunk)
	{
		uint16_t packedId[3] = { UINT16_MAX };
		PackEntityId(entityId, packedId);

		std::memcpy(chunk.GetBuffer() + GetIdEntryOffset(entityIndexInChunk), packedId, PACKED_ENTITY_ID_SIZE);
	}

	Entity EntityStorage::ReadEntityIdEntry(const EntityStorageChunk& chunk, size_t entityIndexInChunk) const
	{
		uint16_t packedId[3] = { UINT16_MAX };
		std::memcpy(packedId, chunk.GetBuffer() + GetIdEntryOffset(entityIndexInChunk), PACKED_ENTITY_ID_SIZE);
		return UnpackEntityId(packedId);
	}
}