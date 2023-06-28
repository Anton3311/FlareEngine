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

		EntityStorageChunk(EntityStorageChunk&) = delete;

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
			: m_EntitySize(0), m_EntitiesCount(0) {}

		size_t AddEntity();
		uint8_t* GetEntityData(size_t entityIndex);
		
		inline size_t GetEntitiesCount() const { return m_EntitiesCount; }
		inline size_t GetEntitySize() const { return m_EntitySize; }
		void SetEntitySize(size_t entitySize);
	public:
		using StorageChunk = EntityStorageChunk<4096>;
	private:
		StorageChunk m_Chunk;

		size_t m_EntitySize;
		size_t m_EntitiesCount;
	};
}