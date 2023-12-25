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
	private:
		T* m_Values;
		size_t m_Size;
	};
}