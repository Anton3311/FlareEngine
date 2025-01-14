#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/Entity/Archetypes.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <stdint.h>

namespace Flare	
{
	struct Components;

	struct EntityStorageRequirements
	{
		constexpr bool IsValid() const { return EntitySize > 0 && EntityAlignment > 0; }

		size_t EntitySize = 0;
		size_t EntityAlignment = 0;
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
			const ArchetypeComponents& archetypeComponents = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype);
			DefaultConstructEntityComponentsRange(entityIndex, 0, archetypeComponents.ComponentCount);
		}

		void DefaultConstructEntityComponentsRange(size_t entityIndex, size_t startComponent, size_t componentCount);

		void CopyConstructEntity(size_t entityIndex, Span<const void*> componentData);
		void MoveConstructEntity(size_t entityIndex, Span<void*> componentData);

		// Uses components' move assignment operator to move entity components from source storage
		void MoveEntityData(size_t sourceEntityIndex, EntityStorage& sourceStorage, size_t destinationEntityIndex);

		void* GetComponentArray(size_t chunkIndex, size_t componentIndex) const
		{
			size_t arrayOffset = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype).ComponentArrayOffsets[componentIndex];
			return m_Chunks[chunkIndex].GetBuffer() + arrayOffset;
		}

		inline const Entity* GetReadonlyEntityIdsArray(size_t chunkIndex) const
		{
			FLARE_CORE_ASSERT(chunkIndex < m_Chunks.size());

			size_t arrayOffset = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype).IdsBufferOffset;
			const void* array = m_Chunks[chunkIndex].GetBuffer() + arrayOffset;

			return (const Entity*)array;
		}
	private:
		inline Entity* GetEntityIdsArray(size_t chunkIndex)
		{
			FLARE_CORE_ASSERT(chunkIndex < m_Chunks.size());

			size_t arrayOffset = m_ArchetypesRegistry->GetArchetypeComponents(m_Archetype).IdsBufferOffset;
			void* array = m_Chunks[chunkIndex].GetBuffer() + arrayOffset;

			return (Entity*)array;
		}

		void ReleaseEntityComponents(size_t entityIndex, const ArchetypeComponents& archetypeComponents);
		void ReleaseAllEntities();

		inline void InvalidateEntityIdEntry(size_t chunkIndex, size_t entityIndexInChunk)
		{
			GetEntityIdsArray(chunkIndex)[entityIndexInChunk] = Entity();
		}

		inline void UpdateEntityIdEntry(size_t chunkIndex, Entity entityId, size_t entityIndexInChunk)
		{
			GetEntityIdsArray(chunkIndex)[entityIndexInChunk] = entityId;
		}

		inline Entity ReadEntityIdEntry(size_t chunkIndex, size_t entityIndexInChunk) const
		{
			return GetReadonlyEntityIdsArray(chunkIndex)[entityIndexInChunk];
		}
	private:
		std::vector<EntityStorageChunk> m_Chunks;

		const Archetypes* m_ArchetypesRegistry = nullptr;
		ArchetypeId m_Archetype = INVALID_ARCHETYPE_ID;

		EntityStorageRequirements m_StorageRequirements;

		size_t m_EntityCount = 0;
		size_t m_EntitiesPerChunk = 0;
	};
}