#pragma once

#include "Flare/Core/Assert.h"

#include "FlareECS/Registry.h"

#include "FlareECS/Query/ComponentView.h"
#include "FlareECS/Query/EntityViewIterator.h"

#include <stdint.h>

namespace Flare
{
	class EntityView
	{
	public:
		EntityView(Registry& registry, size_t archetype)
			: m_Registry(registry), m_Archetype(archetype) {}
	public:
		EntityViewIterator begin();
		EntityViewIterator end();

		template<typename ComponentT>
		ComponentView<ComponentT> View()
		{
			ArchetypeRecord& archetypeRecord = m_Registry.GetArchetypeRecord(m_Archetype);
			std::optional<size_t> index = archetypeRecord.Data.FindComponent(ComponentT::Id);

			FLARE_CORE_ASSERT(index.has_value(), "Archetype doesn't have a component");

			return ComponentView<ComponentT>(archetypeRecord.Data.ComponentOffsets[index.value()]);
		}
	private:
		Registry& m_Registry;
		size_t m_Archetype;
	};
}