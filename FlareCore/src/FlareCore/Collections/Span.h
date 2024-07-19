#pragma once

#include "FlareCore/Assert.h"

namespace Flare
{
	template<typename T>
	class Span
	{
	public:
		Span()
			: m_Values(nullptr), m_Size(0) {}

		Span(T* values, size_t size)
			: m_Values(values), m_Size(size) {}

		explicit Span(T& singleValue)
			: m_Values(&singleValue), m_Size(1) {}

		constexpr bool IsEmpty() const { return m_Values == nullptr || m_Size == 0; }
		constexpr bool IsValid() const { return m_Values != nullptr; }

		constexpr T* begin() { return m_Values; }
		constexpr T* end() { return m_Values + m_Size; }
		constexpr const T* begin() const { return m_Values; }
		constexpr const T* end() const { return m_Values + m_Size; }

		constexpr size_t GetSize() const { return m_Size; }
		constexpr T* GetData() { return m_Values; }
		constexpr const T* GetData() const { return m_Values; }

		bool operator==(const Span<T>& other) const
		{
			if (this == &other)
				return true;

			if (m_Size != other.m_Size)
				return false;

			if (m_Values == other.m_Values)
				return true;

			for (size_t i = 0; i < m_Size; i++)
			{
				if (m_Values[i] != other[i])
				{
					return false;
				}
			}

			return true;
		}

		bool operator!=(const Span<T>& other) const
		{
			if (this == &other)
				return false;

			if (m_Size != other.m_Size)
				return true;

			if (m_Values != other.m_Values)
				return true;

			for (size_t i = 0; i < m_Size; i++)
			{
				if (m_Values[i] != other[i])
				{
					return true;
				}
			}

			return false;
		}

		inline T& operator[](size_t index)
		{
			FLARE_CORE_ASSERT(index < m_Size);
			return m_Values[index];
		}

		inline const T& operator[](size_t index) const
		{
			FLARE_CORE_ASSERT(index < m_Size);
			return m_Values[index];
		}

		inline static Span<T> FromVector(std::vector<T>& vector)
		{
			return Span<T>(vector.data(), vector.size());
		}
	private:
		T* m_Values;
		size_t m_Size;
	};

	class MemorySpan
	{
	public:
		MemorySpan()
			: m_Buffer(nullptr), m_Size(0) {}

		template<typename T>
		MemorySpan(const T* elements, size_t count)
			: m_Buffer(elements), m_Size(count * sizeof(T)) {}

		constexpr bool IsValid() const { return m_Buffer != nullptr; }
		constexpr bool IsEmpty() const { return m_Size == 0 || m_Buffer == nullptr; }

		constexpr const void* GetBuffer() const { return m_Buffer; }

		constexpr size_t GetSize() const { return m_Size; }

		template<typename T>
		inline static MemorySpan FromVector(std::vector<T>& vector)
		{
			return MemorySpan(vector.data(), vector.size());
		}

		inline static MemorySpan FromRawBytes(const void* bytes, size_t size)
		{
			MemorySpan span{};
			span.m_Buffer = bytes;
			span.m_Size = size;

			return span;
		}
	private:
		const void* m_Buffer;
		size_t m_Size;
	};
}