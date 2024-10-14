#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Log.h"

#include "FlareCore/Collections/Span.h"

#include <stdint.h>
#include <stdlib.h>
#include <vector>

#include <initializer_list>

namespace Flare
{
	template<typename T, uint32_t N>
	class StackStorage
	{
	public:
		StackStorage() = default;

		constexpr T* GetData() { return reinterpret_cast<T*>(m_Buffer); }
		constexpr const T* GetData() const { return reinterpret_cast<const T*>(m_Buffer); }
	private:
		alignas(alignof(T)) uint8_t m_Buffer[(size_t)N * sizeof(T)];
	};

	template<typename T>
	class StackStorage<T, 0>
	{
	public:
		constexpr T* GetData() { return nullptr; }
		constexpr const T* GetData() const { return nullptr; }
	};

	template<typename T, uint32_t InlineCapacity>
	class SmallVector
	{
	public:
		SmallVector()
			: m_Array(m_InlineBuffer.GetData()), m_Capacity(InlineCapacity), m_Size(0) {}

		SmallVector(const std::initializer_list<const T>& elements)
			: m_Array(nullptr), m_Capacity(InlineCapacity), m_Size(0)
		{
			ConstructFromArray(elements.begin(), elements.end());
		}

		template<uint32_t N2>
		SmallVector(const SmallVector<T, N2>& other)
		{
			ConstructFromArray(other.begin(), other.end());
		}

		~SmallVector()
		{
			Clear();

			if (!IsUsingInlineBuffer())
				free(m_Array);
		}
		
		constexpr uint32_t GetSize() const { return m_Size; }
		constexpr uint32_t GetCapacity() const { return m_Capacity; }

		constexpr T* GetData() { return m_Array; }
		constexpr const T* GetData() const { return m_Array; }
		
		constexpr T* begin() { return m_Array; }
		constexpr T* end() { return m_Array + m_Size; }
		constexpr const T* begin() const { return m_Array; }
		constexpr const T* end() const { return m_Array + m_Size; }

		constexpr bool IsUsingInlineBuffer() const { return m_Array == m_InlineBuffer.GetData(); }

		inline Span<T> ToSpan() { return Span(m_Array, m_Size); }
		inline Span<const T> ToSpan() const { return Span(m_Array, m_Size); }

		inline void Append(const T& value)
		{
			if (m_Size == m_Capacity)
				EnsureCapacity(ComputeGrowCapacity());

			new(&m_Array[m_Size]) T(value);
			m_Size++;
		}

		inline void Append(T&& value)
		{
			if (m_Size == m_Capacity)
				EnsureCapacity(ComputeGrowCapacity());

			new(&m_Array[m_Size]) T(std::move(value));
			m_Size++;
		}
		
		template<typename... Args>
		inline T& Emplace(Args&& ...args)
		{
			if (m_Size == m_Capacity)
				EnsureCapacity(ComputeGrowCapacity());

			T* element = new(&m_Array[m_Size]) T(std::forward<Args>()...);
			m_Size++;

			return *element;
		}

		void RemoveAt(uint32_t index)
		{
			FLARE_CORE_ASSERT(index < m_Size);

			for (uint32_t i = index + 1; i < m_Size; i++)
			{
				m_Array[i - 1] = m_Array[i];
			}

			m_Array[m_Size - 1].~T();
			m_Size--;
		}

		inline T& operator[](uint32_t index)
		{
			FLARE_CORE_ASSERT(index < m_Size);
			return m_Array[index];
		}

		inline const T& operator[](uint32_t index) const
		{
			FLARE_CORE_ASSERT(index < m_Size);
			return m_Array[index];
		}

		void Clear()
		{
			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_Array[i].~T();
			}

			m_Size = 0;
		}

		void Resize(uint32_t newSize)
		{
			EnsureCapacity(newSize);
			DefaultConstructElementsInRange(m_Size, m_Capacity - m_Size);
			m_Size = m_Capacity;
		}

		void EnsureCapacity(uint32_t newCapacity)
		{
			if (IsUsingInlineBuffer() && newCapacity <= InlineCapacity)
			{
				m_Capacity = newCapacity;
			}
			else if (newCapacity > InlineCapacity)
			{
				T* newArray = (T*)malloc(sizeof(T) * newCapacity);

				for (uint32_t i = 0; i < m_Size; i++)
				{
					new(&newArray[i]) T(std::move(m_Array[i]));
				}

				for (uint32_t i = 0; i < m_Size; i++)
				{
					m_Array[i].~T();
				}

				if (!IsUsingInlineBuffer())
				{
					free(m_Array);
				}
			
				m_Array = newArray;
				m_Capacity = newCapacity;
			}
		}
	private:
		void DefaultConstructElementsInRange(uint32_t start, uint32_t count)
		{
			for (uint32_t i = start; i < start + count; i++)
			{
				new(&m_Array[i]) T();
			}
		}

		void ConstructFromArray(const T* start, const T* end)
		{
			m_Size = (uint32_t)(end - start);

			if (m_Size < InlineCapacity)
			{
				m_Array = m_InlineBuffer.GetData();
			}
			else
			{
				m_Array = (T*)malloc(m_Size * sizeof(T));
				m_Capacity = m_Size;
			}

			for (uint32_t i = 0; i < m_Size; i++)
			{
				new(m_Array[i]) T(start[i]);
			}
		}

		constexpr uint32_t ComputeGrowCapacity() const { return m_Capacity + (m_Capacity + 1) / 2; }
	private:
		T* m_Array;
		StackStorage<T, InlineCapacity> m_InlineBuffer;

		uint32_t m_Capacity;
		uint32_t m_Size;
	};
}
