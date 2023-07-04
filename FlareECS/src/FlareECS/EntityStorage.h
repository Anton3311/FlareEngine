#pragma once

#include "Flare/Core/Assert.h"

#include <stdint.h>

namespace Flare
{
	template<size_t Size>
	class EntityStorageChunk
	{
	public:
		EntityStorageChunk()
			: m_Buffer(nullptr) {}

		EntityStorageChunk(EntityStorageChunk& other)
		{
			if (m_Buffer != nullptr)
				delete[] m_Buffer;

			m_Buffer = other.m_Buffer;
			other.m_Buffer = nullptr;
		}

		EntityStorageChunk(EntityStorageChunk&& other)
		{
			if (m_Buffer != nullptr)
				delete[] m_Buffer;

			m_Buffer = other.m_Buffer;
			other.m_Buffer = nullptr;
		}

		~EntityStorageChunk()
		{			
			if (m_Buffer != nullptr)
				delete[] m_Buffer;

			m_Buffer = nullptr;
		}

		constexpr size_t GetSize() const { return Size; }

		void Allocate()
		{
			if (m_Buffer == nullptr)
				m_Buffer = new uint8_t[Size];
		}

		inline bool IsAllocated() const { return m_Buffer != nullptr; }

		uint8_t* GetBuffer() const { return m_Buffer; }
	private:
		uint8_t* m_Buffer = nullptr;
	};

	class EntityStorage
	{
	public:
		EntityStorage()
			: m_EntitySize(0), m_EntitiesCount(0), m_EntitiesPerChunk(0) {}

		size_t AddEntity(size_t registryIndex);
		uint8_t* GetEntityData(size_t entityIndex);

		void RemoveEntityData(size_t entityIndex);
		
		inline size_t GetEntitiesCount() const { return m_EntitiesCount; }
		inline size_t GetEntitySize() const { return m_EntitySize; }
		void SetEntitySize(size_t entitySize);

		inline const std::vector<size_t>& GetEntityIndices() const { return m_EntityIndices; }
	public:
		static constexpr size_t DefaultStorageChunkSize = 4096;
		using StorageChunk = EntityStorageChunk<DefaultStorageChunkSize>;
	private:
		std::vector<StorageChunk> m_Chunks;
		std::vector<size_t> m_EntityIndices;

		size_t m_EntitySize;
		size_t m_EntitiesCount;
		size_t m_EntitiesPerChunk;
	};
}