#include "EntityStorage.h"

#include "FlareCore/Profiler/Profiler.h"

#include <cmath>

namespace Flare
{
	size_t EntityDataStorage::AddEntity()
	{
		if (m_EntityCount % m_EntitiesPerChunk == 0)
			m_Chunks.push_back(EntityChunksPool::GetInstance()->GetOrCreate());

		m_EntityCount++;
		return m_EntityCount - 1;
	}

	uint8_t* EntityDataStorage::GetEntityData(size_t index) const
	{
		size_t bytesOffset = (index % m_EntitiesPerChunk * m_StorageRequirements.EntitySize);
		size_t chunkIndex = index / m_EntitiesPerChunk;

		FLARE_CORE_ASSERT(bytesOffset + m_StorageRequirements.EntitySize <= m_UsedChunkBytes);
		FLARE_CORE_ASSERT(chunkIndex < m_Chunks.size());

		return m_Chunks[chunkIndex].GetBuffer() + bytesOffset;
	}

	void EntityDataStorage::RemoveEntityData(size_t index)
	{
		FLARE_CORE_ASSERT(index < m_EntityCount);

		if (index != m_EntityCount - 1)
			std::memcpy(GetEntityData(index), GetEntityData(m_EntityCount - 1), m_StorageRequirements.EntitySize);
		m_EntityCount--;

		if (m_EntityCount % m_EntitiesPerChunk == 0)
		{
			EntityChunksPool::GetInstance()->Add(std::move(m_Chunks.back()));
			m_Chunks.erase(m_Chunks.end() - 1);
		}
	}

	size_t EntityDataStorage::GetEntitiesCountInChunk(size_t index) const
	{
		FLARE_CORE_ASSERT(index < m_Chunks.size());
		if (index == m_Chunks.size() - 1)
			return m_EntityCount % m_EntitiesPerChunk;
		return m_EntitiesPerChunk;
	}

	void EntityDataStorage::Initialize(const EntityStorageRequirements& storageRequirements)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(storageRequirements.IsValid());

		m_StorageRequirements = storageRequirements;
		m_EntitiesPerChunk = (size_t)floor((float)m_UsedChunkBytes / (float)m_StorageRequirements.EntitySize);
	}

	void EntityDataStorage::Release()
	{
		FLARE_PROFILE_FUNCTION();

		m_StorageRequirements = {};

		for (EntityStorageChunk& chunk : m_Chunks)
			EntityChunksPool::GetInstance()->Add(std::move(chunk));

		m_Chunks.clear();
		m_EntityCount = 0;
	}



	EntityStorage::EntityStorage() {}

	EntityStorage::EntityStorage(EntityStorage&& other) noexcept
		: m_EntityIds(std::move(other.m_EntityIds)), m_DataStorage(std::move(other.m_DataStorage)) {}

	EntityStorage& EntityStorage::operator=(EntityStorage&& other) noexcept
	{
		m_EntityIds = std::move(other.m_EntityIds);
		m_DataStorage = std::move(other.m_DataStorage);
		
		return *this;
	}

	size_t EntityStorage::AddEntity(Entity entity)
	{
		size_t index = m_DataStorage.AddEntity();
		m_EntityIds.push_back(entity);
		return index;
	}

	uint8_t* EntityStorage::GetEntityData(size_t entityIndex) const
	{
		return m_DataStorage.GetEntityData(entityIndex);
	}

	void EntityStorage::RemoveEntityData(size_t entityIndex)
	{
		FLARE_CORE_ASSERT(entityIndex < m_DataStorage.GetEntityCount());

		Entity lastEntity = m_EntityIds.back();

		m_EntityIds[entityIndex] = lastEntity;
		m_EntityIds.erase(m_EntityIds.end() - 1);

		m_DataStorage.RemoveEntityData(entityIndex);
	}

	void EntityStorage::UpdateEntityRegistryIndex(size_t entityIndex, Entity newId)
	{
		FLARE_CORE_ASSERT(entityIndex < m_EntityIds.size());
		m_EntityIds[entityIndex] = newId;
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