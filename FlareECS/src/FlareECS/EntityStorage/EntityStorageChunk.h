#pragma once

#include "FlarePlatform/Platform.h"

#include <stdint.h>
#include <intrin.h>

namespace Flare
{
	class EntityStorageChunk
	{
	public:
		static constexpr size_t CHUNK_SIZE = 4096;
		static constexpr size_t CHUNK_ALIGNMENT = std::max(alignof(std::max_align_t), alignof(__m128));

		EntityStorageChunk()
			: m_Buffer(nullptr) {}

		EntityStorageChunk(EntityStorageChunk& other)
		{
			if (m_Buffer != nullptr)
				Platform::FreeAligned(m_Buffer);

			m_Buffer = other.m_Buffer;
			other.m_Buffer = nullptr;
		}

		EntityStorageChunk(EntityStorageChunk&& other) noexcept
		{
			if (m_Buffer != nullptr)
				Platform::FreeAligned(m_Buffer);

			m_Buffer = other.m_Buffer;
			other.m_Buffer = nullptr;
		}

		~EntityStorageChunk()
		{
			if (m_Buffer != nullptr)
				Platform::FreeAligned(m_Buffer);

			m_Buffer = nullptr;
		}

		void operator=(EntityStorageChunk& other)
		{
			if (m_Buffer != nullptr)
				Platform::FreeAligned(m_Buffer);

			m_Buffer = other.m_Buffer;
			other.m_Buffer = nullptr;
		}

		void Allocate()
		{
			if (m_Buffer == nullptr)
			{
				m_Buffer = (uint8_t*)Platform::AllocateAligned(CHUNK_SIZE, CHUNK_ALIGNMENT);
			}
		}

		inline bool IsAllocated() const { return m_Buffer != nullptr; }

		uint8_t* GetBuffer() const { return m_Buffer; }
	private:
		uint8_t* m_Buffer = nullptr;
	};
}