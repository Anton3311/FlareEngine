#include "EntityView.h"

namespace Flare
{
	EntityView::EntityView(Entities& entities, ArchetypeId archetype)
		: m_Entities(entities), m_Archetype(archetype) {}

	EntityViewIterator EntityView::begin()
	{
		return EntityViewIterator(m_Entities.GetEntityStorage(m_Archetype), 0);
	}

	EntityViewIterator EntityView::end()
	{
		EntityStorage& storage = m_Entities.GetEntityStorage(m_Archetype);
		return EntityViewIterator(storage, storage.GetEntitiesCount());
	}

	std::optional<Entity> EntityView::GetEntity(size_t index)
	{
		const auto& indices = m_Entities.GetEntityStorage(m_Archetype).GetEntityIndices();
		if (index >= indices.size())
			return {};

		return m_Entities.FindEntityByRegistryIndex(indices[index]);
	}

	ArchetypeId EntityView::GetArchetype() const
	{
		return m_Archetype;
	}
}