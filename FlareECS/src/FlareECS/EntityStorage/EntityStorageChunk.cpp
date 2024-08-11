#include "EntityStorageChunk.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlarePlatform/Platform.h"

namespace Flare 
{
	//
	// EntityStorageChunk
	//

	EntityStorageChunk::EntityStorageChunk()
	{
		m_Buffer = (uint8_t*)Platform::AllocateAligned(CHUNK_SIZE, CHUNK_ALIGNMENT);
	}

	EntityStorageChunk::~EntityStorageChunk()
	{
		if (m_Buffer != nullptr)
			Platform::FreeAligned(m_Buffer);

		m_Buffer = nullptr;
	}

	//
	// EntityChunksPool
	//

	Scope<EntityChunksPool> EntityChunksPool::s_Instance = nullptr;

	EntityChunksPool::EntityChunksPool(size_t capacity)
		: m_Capacity(capacity), m_Count(0)
	{
		FLARE_PROFILE_FUNCTION();
		m_Chunks = new EntityStorageChunk[capacity];
	}

	EntityStorageChunk EntityChunksPool::GetOrCreate()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Count == 0)
		{
			EntityStorageChunk chunk = EntityStorageChunk();
			return chunk;
		}

		EntityStorageChunk chunk = std::move(m_Chunks[m_Count - 1]);
		m_Count--;
		return chunk;
	}

	void EntityChunksPool::Add(EntityStorageChunk&& chunk)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Count == m_Capacity)
		{
			return;
		}

		m_Chunks[m_Count++] = std::move(chunk);
	}

	void EntityChunksPool::Initialize(size_t capacity)
	{
		FLARE_PROFILE_FUNCTION();
		if (s_Instance == nullptr)
			s_Instance = CreateScope<EntityChunksPool>(capacity);
	}

	Scope<EntityChunksPool>& EntityChunksPool::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance != nullptr);
		return s_Instance;
	}
}
