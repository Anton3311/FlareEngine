#pragma once

#include "FlareECS/Registry.h"

namespace Flare
{
	class EntityRegistryIterator
	{
	public:
		constexpr EntityRegistryIterator(Registry& registry, size_t index)
			: m_Registry(registry), m_EntityIndex(index) {}

		inline Entity operator*() const
		{
			return m_Registry.m_EntityRecords[m_EntityIndex].Id;
		}

		constexpr EntityRegistryIterator operator++()
		{
			m_EntityIndex++;
			return *this;
		}

		constexpr bool operator==(const EntityRegistryIterator& other)
		{
			return &m_Registry == &other.m_Registry && m_EntityIndex == other.m_EntityIndex;
		}

		constexpr bool operator!=(const EntityRegistryIterator& other)
		{
			return &m_Registry != &other.m_Registry || m_EntityIndex != other.m_EntityIndex;
		}
	private:
		Registry& m_Registry;
		size_t m_EntityIndex;
	};
}