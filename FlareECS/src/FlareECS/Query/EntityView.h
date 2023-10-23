#pragma once

#include "FlareCore/Assert.h"

#include "FlareECS/Entities.h"

#include "FlareECS/Query/ComponentView.h"
#include "FlareECS/Query/EntityViewIterator.h"

#include <stdint.h>
#include <unordered_set>

namespace Flare
{
	class FLAREECS_API EntityView
	{
	public:
		EntityView(Entities& entities, QueryTarget target, ArchetypeId archetype);
	public:
		EntityViewIterator begin();
		EntityViewIterator end();

		std::optional<Entity> GetEntity(size_t index);

		ArchetypeId GetArchetype() const;

		template<typename ComponentT>
		ComponentView<ComponentT> View()
		{
			const ArchetypeRecord& archetypeRecord = m_Entities.GetArchetypes()[m_Archetype];
			std::optional<size_t> index = m_Entities.GetArchetypes().GetArchetypeComponentIndex(m_Archetype, COMPONENT_ID(ComponentT));

			FLARE_CORE_ASSERT(index.has_value(), "Archetype doesn't have a component");

			return ComponentView<ComponentT>(archetypeRecord.ComponentOffsets[index.value()]);
		}

		template<typename T>
		OptionalComponentView<T> ViewOptional()
		{
			const ArchetypeRecord& archetypeRecord = m_Entities.GetArchetypes()[m_Archetype];
			std::optional<size_t> index = m_Entities.GetArchetypes().GetArchetypeComponentIndex(m_Archetype, COMPONENT_ID(T));

			if (index.has_value())
				return OptionalComponentView<T>(archetypeRecord.ComponentOffsets[index.value()]);
			return OptionalComponentView<T>();
		}
	private:
		QueryTarget m_QueryTarget;
		Entities& m_Entities;
		ArchetypeId m_Archetype;
	};
}