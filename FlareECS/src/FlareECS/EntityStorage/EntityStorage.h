#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/Entity/Entity.h"
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

	struct EntityStorageLayout
	{
		size_t ChunkSize = 0;
		size_t EntityStride = 0;
		size_t IdStride = 0;
		size_t IdOffset = 0;
	};

	class FLAREECS_API EntityStorage
	{
	public:
		EntityStorage() = default;
		EntityStorage(const EntityStorage&) = delete;
		EntityStorage(EntityStorage&& other) noexcept = default;

		EntityStorage& operator=(const EntityStorage&) = delete;
		EntityStorage& operator=(EntityStorage&& other) noexcept = default;

		size_t AddEntity(Entity entity);
		uint8_t* GetEntityData(size_t index) const;
		void RemoveEntity(size_t index);

		size_t GetEntitiesCountInChunk(size_t index) const;

		void Initialize(const EntityStorageRequirements& storageRequirements);
		void Release();

		Entity GetEntityId(size_t entityIndex) const;

		inline const EntityStorageRequirements& GetStorageRequirements() const { return m_StorageRequirements; }
		inline size_t GetEntitySize() const { return m_StorageRequirements.EntitySize; }

		inline size_t GetEntityCount() const { return m_EntityCount; }
		inline size_t GetEntitiesPerChunk() const { return m_EntitiesPerChunk; }
		inline const std::vector<EntityStorageChunk>& GetChunks() const { return m_Chunks; }

		inline uint8_t* GetChunkBuffer(size_t chunkIndex) { return m_Chunks[chunkIndex].GetBuffer(); }

		inline size_t GetChunkCount() const { return m_Chunks.size(); }
		inline const EntityStorageChunk& GetChunk(size_t index) const { return m_Chunks[index]; }
	private:
		static constexpr size_t PACKED_ENTITY_ID_SIZE = sizeof(uint16_t) * 3;

		inline size_t GetIdEntryOffset(size_t entityIndexInChunk) const
		{
			return m_Layout.IdOffset + entityIndexInChunk * m_Layout.IdStride;
		}

		inline size_t GetEntityDataOffset(size_t entityIndexInChunk) const
		{
			return m_Layout.EntityStride * entityIndexInChunk;
		}

		void InvalidateEntityIdEntry(EntityStorageChunk& chunk, size_t entityIndexInChunk);
		void UpdateEntityIdEntry(EntityStorageChunk& chunk, Entity entityId, size_t entityIndexInChunk);
		Entity ReadEntityIdEntry(const EntityStorageChunk& chunk, size_t entityIndexInChunk) const;

		static void PackEntityId(Entity entity, uint16_t outPacked[3]);
		static Entity UnpackEntityId(uint16_t packed[3]);
	private:
		std::vector<EntityStorageChunk> m_Chunks;

		EntityStorageRequirements m_StorageRequirements;
		EntityStorageLayout m_Layout;

		size_t m_EntityCount = 0;
		size_t m_EntitiesPerChunk = 0;
	};
}