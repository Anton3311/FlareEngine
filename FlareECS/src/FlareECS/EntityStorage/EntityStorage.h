#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <stdint.h>

namespace Flare	
{
	struct EntityStorageRequirements
	{
		constexpr bool IsValid() const { return EntitySize > 0 && EntityAlignment > 0; }

		size_t EntitySize = 0;
		size_t EntityAlignment = 0;
	};

	class FLAREECS_API EntityDataStorage
	{
	public:
		EntityDataStorage() = default;
		EntityDataStorage(const EntityDataStorage&) = delete;
		EntityDataStorage(EntityDataStorage&& other) noexcept = default;

		EntityDataStorage& operator=(const EntityDataStorage&) = delete;
		EntityDataStorage& operator=(EntityDataStorage&& other) noexcept = default;

		size_t AddEntity();
		uint8_t* GetEntityData(size_t index) const;
		void RemoveEntityData(size_t index);

		size_t GetEntitiesCountInChunk(size_t index) const;

		void Initialize(const EntityStorageRequirements& storageRequirements);
		void Release();

		inline const EntityStorageRequirements& GetStorageRequirements() const { return m_StorageRequirements; }
		inline size_t GetEntitySize() const { return m_StorageRequirements.EntitySize; }

		inline size_t GetEntityCount() const { return m_EntityCount; }
		inline size_t GetEntitiesPerChunk() const { return m_EntitiesPerChunk; }
		inline const std::vector<EntityStorageChunk>& GetChunks() const { return m_Chunks; }

		inline size_t GetChunkCount() const { return m_Chunks.size(); }
		inline const EntityStorageChunk& GetChunk(size_t index) const { return m_Chunks[index]; }
	private:
		std::vector<EntityStorageChunk> m_Chunks;

		EntityStorageRequirements m_StorageRequirements;

		size_t m_UsedChunkBytes = EntityStorageChunk::CHUNK_SIZE;
		size_t m_EntityCount = 0;
		size_t m_EntitiesPerChunk = 0;
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
		
		inline size_t GetEntitiesCount() const { return m_DataStorage.GetEntityCount(); }
		inline size_t GetEntitySize() const { return m_DataStorage.GetEntitySize(); }

		void Initialize(const EntityStorageRequirements& storageRequirements) { m_DataStorage.Initialize(storageRequirements); }

		void UpdateEntityRegistryIndex(size_t entityIndex, uint32_t newRegistryIndex);

		inline size_t GetChunksCount() const { return m_DataStorage.GetChunkCount(); }
		inline size_t GetEntitiesPerChunkCount() const { return m_DataStorage.GetEntitiesPerChunk(); }

		size_t GetEntitiesCountInChunk(size_t index) const { return m_DataStorage.GetEntitiesCountInChunk(index); }

		uint8_t* GetChunkBuffer(size_t index);
		const uint8_t* GetChunkBuffer(size_t index) const;

		inline const std::vector<uint32_t>& GetEntityIndices() const { return m_EntityIndices; }
	private:
		EntityDataStorage m_DataStorage;
		std::vector<uint32_t> m_EntityIndices;
	};
}