#include "EntityStorage.h"

#include "FlareCore/Profiler/Profiler.h"

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

	uint8_t* EntityStorage::GetEntityData(size_t index) const
	{
		size_t bytesOffset = (index % m_EntitiesPerChunk * m_StorageRequirements.EntitySize);
		size_t chunkIndex = index / m_EntitiesPerChunk;

		FLARE_CORE_ASSERT(bytesOffset + m_StorageRequirements.EntitySize <= m_Layout.ChunkSize);
		FLARE_CORE_ASSERT(chunkIndex < m_Chunks.size());

		return m_Chunks[chunkIndex].GetBuffer() + bytesOffset;
	}

	void EntityStorage::RemoveEntity(size_t index)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(index < m_EntityCount);

		if (index != m_EntityCount - 1)
		{
			// TODO: Use copy assignment
			std::memcpy(GetEntityData(index), GetEntityData(m_EntityCount - 1), m_StorageRequirements.EntitySize);

			Entity lastEntityId = GetEntityId(m_EntityCount - 1);

			size_t chunkIndex = index / m_EntitiesPerChunk;
			size_t indexInChunk = index % m_EntitiesPerChunk;

			UpdateEntityIdEntry(m_Chunks[chunkIndex], lastEntityId, indexInChunk);
			InvalidateEntityIdEntry(m_Chunks.back(), (m_EntityCount - 1) % m_EntitiesPerChunk);
		}

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

	void EntityStorage::Initialize(const EntityStorageRequirements& storageRequirements)
	{
		FLARE_PROFILE_FUNCTION();

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

		m_StorageRequirements = {};

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

	void EntityStorage::PackEntityId(Entity entity, uint16_t outPacked[3])
	{
		outPacked[0] = (uint16_t)(entity.GetIndex() & 0xffff);
		outPacked[1] = (uint16_t)((entity.GetIndex() >> 16) & 0xffff);
		outPacked[2] = entity.GetGeneration();
	}

	Entity EntityStorage::UnpackEntityId(uint16_t packed[3])
	{
		uint32_t index = (uint32_t)packed[0] | ((uint32_t)packed[1] << 16);
		return Entity(index, packed[2]);
	}
}