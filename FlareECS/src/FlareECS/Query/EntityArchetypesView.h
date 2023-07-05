#pragma once

#include "FlareECS/Registry.h"

#include "FlareECS/Query/EntityView.h"

#include <vector>

namespace Flare
{
	class EntityArchetypesIterator
	{
	public:
		EntityArchetypesIterator(Registry& registry, const std::unordered_set<ArchetypeId>::const_iterator& archetype)
			: m_Registry(registry), m_Archetype(archetype) {}

		inline EntityView operator*()
		{
			return EntityView(m_Registry, *m_Archetype);
		}

		inline EntityArchetypesIterator operator++()
		{
			m_Archetype++;
			return *this;
		}

		inline bool operator==(const EntityArchetypesIterator& other)
		{
			return &m_Registry == &other.m_Registry && m_Archetype == other.m_Archetype;
		}

		inline bool operator!=(const EntityArchetypesIterator& other)
		{
			return &m_Registry != &other.m_Registry || m_Archetype != other.m_Archetype;
		}
	private:
		Registry& m_Registry;
		std::unordered_set<ArchetypeId>::const_iterator m_Archetype;
	};

	class EntityArchetypesView
	{
	public:
		EntityArchetypesView(Registry& registry, const std::unordered_set<ArchetypeId>& archetypes)
			: m_Registry(registry), m_Archetypes(archetypes) {}

		inline EntityArchetypesIterator begin()
		{
			return EntityArchetypesIterator(m_Registry, m_Archetypes.begin());
		}

		inline EntityArchetypesIterator end()
		{
			return EntityArchetypesIterator(m_Registry, m_Archetypes.end());
		}
	private:
		Registry& m_Registry;
		const std::unordered_set<ArchetypeId>& m_Archetypes;
	};
}