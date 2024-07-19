#include "EntityStorage.h"

#include "FlareECS/EntityStorage/EntityChunksPool.h"

#include <cmath>

namespace Flare
{
	EntityDataStorage::EntityDataStorage()
		: EntitySize(0), EntitiesCount(0), EntitiesPerChunk(0) {}
	
	EntityDataStorage::EntityDataStorage(EntityDataStorage&& other) noexcept
		: Chunks(std::move(other.Chunks)),
		EntitySize(other.EntitySize), EntitiesPerChunk(other.EntitiesPerChunk), EntitiesCount(other.EntitiesCount)
	{
		other.EntitiesCount = 0;
		other.EntitySize = 0;
		other.EntitiesPerChunk = 0;
	}

	EntityDataStorage& EntityDataStorage::operator=(EntityDataStorage&& other) noexcept
	{
		Chunks = std::move(other.Chunks);
		EntitySize = other.EntitySize;
		EntitiesPerChunk = other.EntitiesPerChunk;
		EntitiesCount = other.EntitiesCount;

		other.EntitySize = 0;
		other.EntitiesPerChunk = 0;
		other.EntitiesCount = 0;

		return *this;
	}

	size_t EntityDataStorage::AddEntity()
	{
		FLARE_CORE_ASSERT(EntitySize > 0, "Entity has no size");

		if (EntitiesCount % EntitiesPerChunk == 0)
			Chunks.push_back(EntityChunksPool::GetInstance()->GetOrCreate());

		EntitiesCount++;
		return EntitiesCount - 1;
	}

	uint8_t* EntityDataStorage::GetEntityData(size_t index) const
	{
		size_t bytesOffset = (index % EntitiesPerChunk * EntitySize);
		size_t chunkIndex = index / EntitiesPerChunk;

		FLARE_CORE_ASSERT(bytesOffset <= ENTITY_CHUNK_SIZE - EntitySize);
		FLARE_CORE_ASSERT(chunkIndex < Chunks.size());

		return Chunks[chunkIndex].GetBuffer() + bytesOffset;
	}

	void EntityDataStorage::RemoveEntityData(size_t index)
	{
		FLARE_CORE_ASSERT(index < EntitiesCount);

		if (index != EntitiesCount - 1)
			std::memcpy(GetEntityData(index), GetEntityData(EntitiesCount - 1), EntitySize);
		EntitiesCount--;

		if (EntitiesCount % EntitiesPerChunk == 0)
		{
			EntityChunksPool::GetInstance()->Add(Chunks.back());
			Chunks.erase(Chunks.end() - 1);
		}
	}

	void EntityDataStorage::SetEntitySize(size_t entitySize)
	{
		FLARE_CORE_ASSERT(EntitiesCount == 0, "Entity size can only be set if the storage is empty");
		EntitySize = entitySize;
		EntitiesPerChunk = (size_t)floor((float)ENTITY_CHUNK_SIZE / (float)entitySize);
	}

	size_t EntityDataStorage::GetEntitiesCountInChunk(size_t index) const
	{
		FLARE_CORE_ASSERT(index < Chunks.size());
		if (index == Chunks.size() - 1)
			return EntitiesCount % EntitiesPerChunk;
		return EntitiesPerChunk;
	}

	void EntityDataStorage::Clear()
	{
		EntitiesCount = 0;
		for (EntityStorageChunk& chunk : Chunks)
			EntityChunksPool::GetInstance()->Add(chunk);

		Chunks.clear();
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
		FLARE_CORE_ASSERT(entityIndex < m_DataStorage.EntitiesCount);

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
		FLARE_CORE_ASSERT(index < m_DataStorage.Chunks.size());
		return m_DataStorage.Chunks[index].GetBuffer();
	}

	const uint8_t* EntityStorage::GetChunkBuffer(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_DataStorage.Chunks.size());
		return m_DataStorage.Chunks[index].GetBuffer();
	}
}