#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Types.h"

#include <stdint.h>
#include <intrin.h>

namespace Flare
{
	//
	// EntityStorageChunk
	//

	class FLAREECS_API EntityStorageChunk
	{
	public:
		static constexpr size_t CHUNK_SIZE = 4096;
		static constexpr size_t CHUNK_ALIGNMENT = std::max(alignof(std::max_align_t), alignof(__m128));

		static_assert(CHUNK_SIZE < std::numeric_limits<EntitySizeT>::max());

		EntityStorageChunk();
		~EntityStorageChunk();

		EntityStorageChunk(const EntityStorageChunk& other) = delete;
		EntityStorageChunk(EntityStorageChunk&& other) noexcept
		{
			std::swap(m_Buffer, other.m_Buffer);
		}

		void operator=(const EntityStorageChunk& other) = delete;
		inline void operator=(EntityStorageChunk&& other) noexcept
		{
			std::swap(m_Buffer, other.m_Buffer);
		}

		constexpr bool IsAllocated() const { return m_Buffer != nullptr; }
		constexpr uint8_t* GetBuffer() const { return m_Buffer; }
	private:
		uint8_t* m_Buffer = nullptr;
	};

	//
	// EntityChunksPool
	//

	class FLAREECS_API EntityChunksPool
	{
	public:
		EntityChunksPool(size_t capacity);

		EntityStorageChunk GetOrCreate();
		void Add(EntityStorageChunk&& chunk);

		inline size_t GetCount() const { return m_Count; }
		inline size_t GetCapacity() const { return m_Capacity; }
	public:
		static void Initialize(size_t capacity);
		static Scope<EntityChunksPool>& GetInstance();
	private:
		size_t m_Capacity;
		size_t m_Count;

		EntityStorageChunk* m_Chunks;
	private:
		static Scope<EntityChunksPool> s_Instance;
	};
}