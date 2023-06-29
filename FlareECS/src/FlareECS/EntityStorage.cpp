#include "EntityStorage.h"

namespace Flare
{
	size_t EntityStorage::AddEntity(size_t registryIndex)
	{
		FLARE_CORE_ASSERT(m_EntitySize > 0, "Entity has no size");

		if (!m_Chunk.IsAllocated())
			m_Chunk.Allocate();

		size_t index = m_EntitiesCount;
		size_t offset = m_EntitySize * m_EntitiesCount;

		FLARE_CORE_ASSERT(offset < m_Chunk.GetSize());
		FLARE_CORE_ASSERT(offset + m_EntitySize <= m_Chunk.GetSize());

		m_EntityIndices.push_back(registryIndex);

		m_EntitiesCount++;
		return index;
	}

	uint8_t* EntityStorage::GetEntityData(size_t entityIndex)
	{
		size_t bytesOffset = entityIndex * m_EntitySize;

		FLARE_CORE_ASSERT(m_Chunk.IsAllocated());
		FLARE_CORE_ASSERT(bytesOffset < m_Chunk.GetSize());

		return m_Chunk.GetBuffer() + bytesOffset;
	}

	void EntityStorage::RemoveEntityData(size_t entityIndex)
	{
		FLARE_CORE_ASSERT(entityIndex < m_EntitiesCount);

		size_t lastEntityIndex = m_EntityIndices.back();

		m_EntityIndices[entityIndex] = lastEntityIndex;
		m_EntityIndices.erase(m_EntityIndices.end() - 1);

		if (entityIndex != m_EntitiesCount - 1)
			std::memcpy(GetEntityData(entityIndex), GetEntityData(m_EntitiesCount - 1), m_EntitySize);
		m_EntitiesCount--;
	}

	void EntityStorage::SetEntitySize(size_t entitySize)
	{
		FLARE_CORE_ASSERT(!(m_Chunk.IsAllocated() && m_EntitiesCount > 0), "Entity size can only be set if the storage is empty");
		m_EntitySize = entitySize;
	}
}