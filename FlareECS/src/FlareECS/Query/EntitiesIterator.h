#pragma once

#include "FlareECS/Entities.h"

namespace Flare
{
	class EntitiesIterator
	{
	public:
		constexpr EntitiesIterator(Entities& entities, size_t index)
			: m_Entities(entities), m_EntityIndex(index) {}

		inline Entity operator*() const
		{
			return m_Entities.m_EntityRecords[m_EntityIndex].Id;
		}

		constexpr EntitiesIterator operator++()
		{
			m_EntityIndex++;
			return *this;
		}

		constexpr bool operator==(const EntitiesIterator& other)
		{
			return &m_Entities == &other.m_Entities && m_EntityIndex == other.m_EntityIndex;
		}

		constexpr bool operator!=(const EntitiesIterator& other)
		{
			return &m_Entities != &other.m_Entities || m_EntityIndex != other.m_EntityIndex;
		}
	private:
		Entities& m_Entities;
		size_t m_EntityIndex;
	};
}