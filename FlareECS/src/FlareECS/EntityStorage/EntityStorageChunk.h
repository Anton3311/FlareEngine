#pragma once

#include <stdint.h>

namespace Flare
{
	constexpr size_t ENTITY_CHUNK_SIZE = 4096;

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

		EntityStorageChunk(EntityStorageChunk&& other) noexcept
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

		void operator=(EntityStorageChunk& other)
		{
			if (m_Buffer != nullptr)
				delete[] m_Buffer;

			m_Buffer = other.m_Buffer;
			other.m_Buffer = nullptr;
		}

		void Allocate()
		{
			if (m_Buffer == nullptr)
				m_Buffer = new uint8_t[ENTITY_CHUNK_SIZE];
		}

		inline bool IsAllocated() const { return m_Buffer != nullptr; }

		uint8_t* GetBuffer() const { return m_Buffer; }
	private:
		uint8_t* m_Buffer = nullptr;
	};
}