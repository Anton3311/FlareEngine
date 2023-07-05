#pragma once

#include "Flare/Core/Assert.h"

#include "FlareECS/Registry.h"

#include "FlareECS/Query/ComponentView.h"
#include "FlareECS/Query/EntityViewIterator.h"

#include <stdint.h>
#include <unordered_set>

namespace Flare
{
	class EntityView
	{
	public:
		EntityView(Registry& registry, ArchetypeId archetype)
			: m_Registry(registry), m_Archetype(archetype) {}
	public:
		EntityViewIterator begin();
		EntityViewIterator end();

		template<typename ComponentT>
		ComponentView<ComponentT> View()
		{
			ArchetypeRecord& archetypeRecord = m_Registry.GetArchetypeRecord(m_Archetype);
			std::optional<size_t> index = m_Registry.GetArchetypeComponentIndex(m_Archetype, ComponentT::Id);

			FLARE_CORE_ASSERT(index.has_value(), "Archetype doesn't have a component");

			return ComponentView<ComponentT>(archetypeRecord.Data.ComponentOffsets[index.value()]);
		}
	private:
		Registry& m_Registry;
		ArchetypeId m_Archetype;
	};
}