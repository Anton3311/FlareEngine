#include "EntityStorage.h"

#include "FlareECS/EntityStorage/EntityChunksPool.h"

#include <cmath>

namespace Flare
{
	EntityStorage::EntityStorage()
		: m_EntitySize(0), m_EntitiesCount(0), m_EntitiesPerChunk(0) {}
	
	EntityStorage::EntityStorage(EntityStorage&& other) noexcept
		: m_Chunks(std::move(other.m_Chunks)), m_EntityIndices(std::move(other.m_EntityIndices)),
		m_EntitySize(other.m_EntitySize), m_EntitiesPerChunk(other.m_EntitiesPerChunk), m_EntitiesCount(other.m_EntitiesCount)
	{
		other.m_EntitiesCount = 0;
		other.m_EntitySize = 0;
		other.m_EntitiesPerChunk = 0;
	}

	size_t EntityStorage::AddEntity(uint32_t registryIndex)
	{
		FLARE_CORE_ASSERT(m_EntitySize > 0, "Entity has no size");

		if (m_EntitiesCount % m_EntitiesPerChunk == 0)
			m_Chunks.push_back(EntityChunksPool::GetInstance()->GetOrCreate());

		size_t index = m_EntitiesCount;
		size_t offset = (m_EntitySize * m_EntitiesCount) % ENTITY_CHUNK_SIZE;
		size_t chunkIndex = m_EntitiesCount / m_EntitiesPerChunk;

		m_EntityIndices.push_back(registryIndex);

		m_EntitiesCount++;
		return index;
	}

	uint8_t* EntityStorage::GetEntityData(size_t entityIndex) const
	{
		size_t bytesOffset = (entityIndex % m_EntitiesPerChunk * m_EntitySize);
		size_t chunkIndex = entityIndex / m_EntitiesPerChunk;

		FLARE_CORE_ASSERT(bytesOffset < ENTITY_CHUNK_SIZE - m_EntitySize);
		FLARE_CORE_ASSERT(chunkIndex < m_Chunks.size());

		return m_Chunks[chunkIndex].GetBuffer() + bytesOffset;
	}

	void EntityStorage::RemoveEntityData(size_t entityIndex)
	{
		FLARE_CORE_ASSERT(entityIndex < m_EntitiesCount);

		uint32_t lastEntityIndex = m_EntityIndices.back();

		m_EntityIndices[entityIndex] = lastEntityIndex;
		m_EntityIndices.erase(m_EntityIndices.end() - 1);

		if (entityIndex != m_EntitiesCount - 1)
			std::memcpy(GetEntityData(entityIndex), GetEntityData(m_EntitiesCount - 1), m_EntitySize);
		m_EntitiesCount--;

		if (m_EntitiesCount % m_EntitiesPerChunk == 0)
		{
			EntityChunksPool::GetInstance()->Add(m_Chunks.back());
			m_Chunks.erase(m_Chunks.end() - 1);
		}
	}

	size_t EntityStorage::GetEntitiesCount() const
	{
		return m_EntitiesCount;
	}

	size_t EntityStorage::GetEntitySize() const
	{
		return m_EntitySize;
	}

	void EntityStorage::SetEntitySize(size_t entitySize)
	{
		FLARE_CORE_ASSERT(m_EntitiesCount == 0, "Entity size can only be set if the storage is empty");
		m_EntitySize = entitySize;
		m_EntitiesPerChunk = (size_t)floor((float) ENTITY_CHUNK_SIZE / (float) entitySize);
	}

	void EntityStorage::UpdateEntityRegistryIndex(size_t entityIndex, uint32_t newRegistryIndex)
	{
		FLARE_CORE_ASSERT(entityIndex < m_EntityIndices.size());
		m_EntityIndices[entityIndex] = newRegistryIndex;
	}

	size_t EntityStorage::GetChunksCount() const
	{
		return m_Chunks.size();
	}

	size_t EntityStorage::GetEntitiesCountInChunk(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_Chunks.size());
		if (index == m_Chunks.size() - 1)
			return m_EntitiesCount % m_EntitiesPerChunk;
		return m_EntitiesPerChunk;
	}

	uint8_t* EntityStorage::GetChunkBuffer(size_t index)
	{
		FLARE_CORE_ASSERT(index < m_Chunks.size());
		return m_Chunks[index].GetBuffer();
	}

	const uint8_t* EntityStorage::GetChunkBuffer(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_Chunks.size());
		return m_Chunks[index].GetBuffer();
	}

	const std::vector<uint32_t>& EntityStorage::GetEntityIndices() const
	{
		return m_EntityIndices;
	}

}