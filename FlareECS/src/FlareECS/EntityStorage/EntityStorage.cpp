#include "EntityStorage.h"

#include "FlareECS/EntityStorage/EntityChunksPool.h"

#include <cmath>

namespace Flare
{
	EntityDataStorage::EntityDataStorage(EntityDataStorage&& other) noexcept
		: m_Chunks(std::move(other.m_Chunks)),
		m_EntitySize(other.m_EntitySize),
		m_EntitiesPerChunk(other.m_EntitiesPerChunk),
		m_EntityCount(other.m_EntityCount),
		m_UsedChunkBytes(other.m_UsedChunkBytes)
	{
		other.m_UsedChunkBytes = 0;
		other.m_EntityCount = 0;
		other.m_EntitySize = 0;
		other.m_EntitiesPerChunk = 0;
	}

	EntityDataStorage& EntityDataStorage::operator=(EntityDataStorage&& other) noexcept
	{
		m_UsedChunkBytes = other.m_UsedChunkBytes;
		m_Chunks = std::move(other.m_Chunks);
		m_EntitySize = other.m_EntitySize;
		m_EntitiesPerChunk = other.m_EntitiesPerChunk;
		m_EntityCount = other.m_EntityCount;

		other.m_UsedChunkBytes = 0;
		other.m_EntitySize = 0;
		other.m_EntitiesPerChunk = 0;
		other.m_EntityCount = 0;

		return *this;
	}

	size_t EntityDataStorage::AddEntity()
	{
		FLARE_CORE_ASSERT(m_EntitySize > 0, "Entity has no size");

		if (m_EntityCount % m_EntitiesPerChunk == 0)
			m_Chunks.push_back(EntityChunksPool::GetInstance()->GetOrCreate());

		m_EntityCount++;
		return m_EntityCount - 1;
	}

	uint8_t* EntityDataStorage::GetEntityData(size_t index) const
	{
		size_t bytesOffset = (index % m_EntitiesPerChunk * m_EntitySize);
		size_t chunkIndex = index / m_EntitiesPerChunk;

		FLARE_CORE_ASSERT(bytesOffset + m_EntitySize <= m_UsedChunkBytes);
		FLARE_CORE_ASSERT(chunkIndex < m_Chunks.size());

		return m_Chunks[chunkIndex].GetBuffer() + bytesOffset;
	}

	void EntityDataStorage::RemoveEntityData(size_t index)
	{
		FLARE_CORE_ASSERT(index < m_EntityCount);

		if (index != m_EntityCount - 1)
			std::memcpy(GetEntityData(index), GetEntityData(m_EntityCount - 1), m_EntitySize);
		m_EntityCount--;

		if (m_EntityCount % m_EntitiesPerChunk == 0)
		{
			EntityChunksPool::GetInstance()->Add(m_Chunks.back());
			m_Chunks.erase(m_Chunks.end() - 1);
		}
	}

	void EntityDataStorage::SetEntitySize(size_t entitySize)
	{
		FLARE_CORE_ASSERT(m_EntityCount == 0, "Entity size can only be set if the storage is empty");
		FLARE_CORE_ASSERT(m_UsedChunkBytes);

		m_EntitySize = entitySize;
		m_EntitiesPerChunk = (size_t)floor((float)m_UsedChunkBytes / (float)entitySize);
	}

	size_t EntityDataStorage::GetEntitiesCountInChunk(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_Chunks.size());
		if (index == m_Chunks.size() - 1)
			return m_EntityCount % m_EntitiesPerChunk;
		return m_EntitiesPerChunk;
	}

	void EntityDataStorage::Clear()
	{
		m_EntityCount = 0;
		for (EntityStorageChunk& chunk : m_Chunks)
			EntityChunksPool::GetInstance()->Add(chunk);

		m_Chunks.clear();
	}



	EntityStorage::EntityStorage() {}

	EntityStorage::EntityStorage(EntityStorage&& other) noexcept
		: m_EntityIndices(std::move(other.m_EntityIndices)), m_DataStorage(std::move(other.m_DataStorage)) {}

	EntityStorage& EntityStorage::operator=(EntityStorage&& other) noexcept
	{
		m_EntityIndices = std::move(other.m_EntityIndices);
		m_DataStorage = std::move(other.m_DataStorage);
		
		return *this;
	}

	size_t EntityStorage::AddEntity(uint32_t registryIndex)
	{
		size_t index = m_DataStorage.AddEntity();
		m_EntityIndices.push_back(registryIndex);
		return index;
	}

	uint8_t* EntityStorage::GetEntityData(size_t entityIndex) const
	{
		return m_DataStorage.GetEntityData(entityIndex);
	}

	void EntityStorage::RemoveEntityData(size_t entityIndex)
	{
		FLARE_CORE_ASSERT(entityIndex < m_DataStorage.GetEntityCount());

		uint32_t lastEntityIndex = m_EntityIndices.back();

		m_EntityIndices[entityIndex] = lastEntityIndex;
		m_EntityIndices.erase(m_EntityIndices.end() - 1);

		m_DataStorage.RemoveEntityData(entityIndex);
	}

	void EntityStorage::SetEntitySize(size_t entitySize)
	{
		m_DataStorage.SetEntitySize(entitySize);
	}

	void EntityStorage::UpdateEntityRegistryIndex(size_t entityIndex, uint32_t newRegistryIndex)
	{
		FLARE_CORE_ASSERT(entityIndex < m_EntityIndices.size());
		m_EntityIndices[entityIndex] = newRegistryIndex;
	}

	uint8_t* EntityStorage::GetChunkBuffer(size_t index)
	{
		FLARE_CORE_ASSERT(index < m_DataStorage.GetChunkCount());
		return m_DataStorage.GetChunk(index).GetBuffer();
	}

	const uint8_t* EntityStorage::GetChunkBuffer(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_DataStorage.GetChunkCount());
		return m_DataStorage.GetChunk(index).GetBuffer();
	}
}