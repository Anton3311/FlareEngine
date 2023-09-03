#include "EntityChunksPool.h"

#include "FlareCore/Assert.h"

namespace Flare
{
	Scope<EntityChunksPool> EntityChunksPool::s_Instance = nullptr;

	EntityChunksPool::EntityChunksPool(size_t capacity)
		: m_Capacity(capacity), m_Count(0)
	{
		m_Chunks = new EntityStorageChunk[capacity];
	}

	EntityStorageChunk EntityChunksPool::GetOrCreate()
	{
		if (m_Count == 0)
		{
			EntityStorageChunk chunk = EntityStorageChunk();
			chunk.Allocate();
			return chunk;
		}

		EntityStorageChunk chunk = m_Chunks[m_Count - 1];
		m_Count--;
		return chunk;
	}

	void EntityChunksPool::Add(EntityStorageChunk& chunk)
	{
		if (m_Count == m_Capacity)
		{
			chunk.~EntityStorageChunk();
			return;
		}

		m_Chunks[m_Count++] = chunk;
	}

	void EntityChunksPool::Initialize(size_t capacity)
	{
		if (s_Instance == nullptr)
			s_Instance = CreateScope<EntityChunksPool>(capacity);
	}

	Scope<EntityChunksPool>& EntityChunksPool::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance != nullptr);
		return s_Instance;
	}
}