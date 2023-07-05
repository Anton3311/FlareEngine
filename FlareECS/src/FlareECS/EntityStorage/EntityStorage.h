#pragma once

#include "Flare/Core/Assert.h"

#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <stdint.h>

namespace Flare
{
	class EntityStorage
	{
	public:
		EntityStorage();
		EntityStorage(EntityStorage&& other) noexcept;

		size_t AddEntity(size_t registryIndex);
		uint8_t* GetEntityData(size_t entityIndex);

		void RemoveEntityData(size_t entityIndex);
		
		inline size_t GetEntitiesCount() const { return m_EntitiesCount; }
		inline size_t GetEntitySize() const { return m_EntitySize; }
		void SetEntitySize(size_t entitySize);
		void UpdateEntityRegistryIndex(size_t entityIndex, size_t newRegistryIndex);

		inline const std::vector<size_t>& GetEntityIndices() const { return m_EntityIndices; }
	private:
		std::vector<EntityStorageChunk> m_Chunks;
		std::vector<size_t> m_EntityIndices;

		size_t m_EntitySize;
		size_t m_EntitiesCount;
		size_t m_EntitiesPerChunk;
	};
}