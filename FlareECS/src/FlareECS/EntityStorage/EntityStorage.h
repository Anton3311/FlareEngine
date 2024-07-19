#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <stdint.h>

namespace Flare	
{
	struct FLAREECS_API EntityDataStorage
	{
	public:
		EntityDataStorage();
		EntityDataStorage(const EntityDataStorage&) = delete;
		EntityDataStorage(EntityDataStorage&& other) noexcept;

		EntityDataStorage& operator=(const EntityDataStorage&) = delete;
		EntityDataStorage& operator=(EntityDataStorage&& other) noexcept;

		size_t AddEntity();
		uint8_t* GetEntityData(size_t index) const;
		void RemoveEntityData(size_t index);

		void SetEntitySize(size_t entitySize);
		size_t GetEntitiesCountInChunk(size_t index) const;

		void Clear();

		std::vector<EntityStorageChunk> Chunks;

		size_t EntitySize;
		size_t EntitiesCount;
		size_t EntitiesPerChunk;
	};

	class FLAREECS_API EntityStorage
	{
	public:
		EntityStorage();
		EntityStorage(const EntityStorage&) = delete;
		EntityStorage(EntityStorage&& other) noexcept;

		EntityStorage& operator=(const EntityStorage&) = delete;
		EntityStorage& operator=(EntityStorage&& other) noexcept;
		
		size_t AddEntity(uint32_t registryIndex);
		uint8_t* GetEntityData(size_t entityIndex) const;

		void RemoveEntityData(size_t entityIndex);

		EntityDataStorage& GetDataStorage() { return m_DataStorage; }
		
		inline size_t GetEntitiesCount() const { return m_DataStorage.EntitiesCount; }
		inline size_t GetEntitySize() const { return m_DataStorage.EntitySize; }

		void SetEntitySize(size_t entitySize);
		void UpdateEntityRegistryIndex(size_t entityIndex, uint32_t newRegistryIndex);

		inline size_t GetChunksCount() const { return m_DataStorage.Chunks.size(); }
		inline size_t GetEntitiesPerChunkCount() const { return m_DataStorage.EntitiesPerChunk; }

		size_t GetEntitiesCountInChunk(size_t index) const { return m_DataStorage.GetEntitiesCountInChunk(index); }

		uint8_t* GetChunkBuffer(size_t index);
		const uint8_t* GetChunkBuffer(size_t index) const;

		inline const std::vector<uint32_t>& GetEntityIndices() const { return m_EntityIndices; }
	private:
		EntityDataStorage m_DataStorage;
		std::vector<uint32_t> m_EntityIndices;
	};
}