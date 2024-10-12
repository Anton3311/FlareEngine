#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/Entity/Archetype.h"
#include "FlareECS/Entity/Archetypes.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <stdint.h>

namespace Flare	
{
	struct Components;

	inline void PackEntityId(Entity entity, uint16_t outPacked[3])
	{
		outPacked[0] = (uint16_t)(entity.GetIndex() & 0xffff);
		outPacked[1] = (uint16_t)((entity.GetIndex() >> 16) & 0xffff);
		outPacked[2] = entity.GetGeneration();
	}

	inline Entity UnpackEntityId(uint16_t packed[3])
	{
		uint32_t index = (uint32_t)packed[0] | ((uint32_t)packed[1] << 16);
		return Entity(index, packed[2]);
	}

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

	class EntityDataGetter
	{
	public:
		EntityDataGetter() = default;
		EntityDataGetter(EntityStorageChunk& chunk, size_t stride)
			: m_Chunk(&chunk), m_Stride(stride) {}

		constexpr uint8_t* GetData(size_t entityIndexInChunk)
		{
			return m_Chunk->GetBuffer() + entityIndexInChunk * m_Stride;
		}

		constexpr const uint8_t* GetData(size_t entityIndexInChunk) const
		{
			return m_Chunk->GetBuffer() + entityIndexInChunk * m_Stride;
		}
	private:
		EntityStorageChunk* m_Chunk = nullptr;
		size_t m_Stride = 0;
	};

	class EntityIdGetter
	{
	public:
		EntityIdGetter() = default;
		EntityIdGetter(const EntityStorageChunk& chunk, size_t offset, size_t stride)
			: m_Chunk(&chunk), m_Offset(offset), m_Stride(stride) {}

		Entity GetEntityId(size_t entityIndexInChunk) const
		{
			const uint8_t* idData = m_Chunk->GetBuffer() + m_Offset + entityIndexInChunk * m_Stride;

			uint16_t packedId[3] = { UINT16_MAX };
			std::memcpy(packedId, idData, sizeof(packedId));

			return UnpackEntityId(packedId);
		}
	private:
		const EntityStorageChunk* m_Chunk;
		size_t m_Offset = 0;
		size_t m_Stride = 0;
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
		void RemoveEntity(size_t index);

		size_t GetEntitiesCountInChunk(size_t index) const;

		void Initialize(const EntityStorageRequirements& storageRequirements, const Archetypes& archetypes, ArchetypeId archetype);
		void Release();

		Entity GetEntityId(size_t entityIndex) const;

		inline EntityDataGetter CreateEntityDataGetter(size_t chunkIndex)
		{
			return EntityDataGetter(m_Chunks[chunkIndex], m_Layout.EntityStride);
		}

		inline EntityIdGetter CreateEntityIdGetter(size_t chunkIndex)
		{
			return EntityIdGetter(m_Chunks[chunkIndex], m_Layout.IdOffset, m_Layout.IdStride);
		}

		void* GetEntityComponentData(size_t entityIndex, size_t componentIndex) const;

		inline const EntityStorageRequirements& GetStorageRequirements() const { return m_StorageRequirements; }
		inline size_t GetEntitySize() const { return m_StorageRequirements.EntitySize; }

		inline size_t GetEntityCount() const { return m_EntityCount; }
		inline size_t GetEntitiesPerChunk() const { return m_EntitiesPerChunk; }
		inline const std::vector<EntityStorageChunk>& GetChunks() const { return m_Chunks; }

		inline uint8_t* GetChunkBuffer(size_t chunkIndex) { return m_Chunks[chunkIndex].GetBuffer(); }

		inline size_t GetChunkCount() const { return m_Chunks.size(); }
		inline const EntityStorageChunk& GetChunk(size_t index) const { return m_Chunks[index]; }

		// Entity data operations
		inline void DefaultConstructEntity(size_t entityIndex)
		{
			const ArchetypeRecord& archetypeRecord = m_ArchetypesRegistry->operator[](m_Archetype);
			DefaultConstructEntityComponentsRange(entityIndex, 0, archetypeRecord.Components.size());
		}

		void DefaultConstructEntityComponentsRange(size_t entityIndex, size_t startComponent, size_t componentCount);

		void CopyConstructEntity(size_t entityIndex, Span<const void*> componentData);
		void MoveConstructEntity(size_t entityIndex, Span<void*> componentData);

		// Uses components' move assignment operator to move entity components from source storage
		void MoveEntityData(size_t sourceEntityIndex, EntityStorage& sourceStorage, size_t destinationEntityIndex);

		void* GetComponentArray(size_t chunkIndex, size_t componentIndex) const
		{
			size_t arrayOffset = m_ArchetypesRegistry->operator[](m_Archetype).ComponentArrayOffsets[componentIndex];
			return m_Chunks[chunkIndex].GetBuffer() + arrayOffset;
		}
	private:
		static constexpr size_t PACKED_ENTITY_ID_SIZE = sizeof(uint16_t) * 3;

		void ReleaseEntityComponents(size_t entityIndex);
		void ReleaseAllEntities();

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
	private:
		std::vector<EntityStorageChunk> m_Chunks;

		const Archetypes* m_ArchetypesRegistry = nullptr;
		ArchetypeId m_Archetype = INVALID_ARCHETYPE_ID;

		EntityStorageRequirements m_StorageRequirements;
		EntityStorageLayout m_Layout;

		size_t m_EntityCount = 0;
		size_t m_EntitiesPerChunk = 0;
	};
}