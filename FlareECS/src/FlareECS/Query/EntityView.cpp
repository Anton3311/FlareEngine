#include "EntityView.h"

namespace Flare
{
	EntityView::EntityView(Entities& entities, QueryTarget target, ArchetypeId archetype)
		: m_Entities(entities), m_Archetype(archetype), m_QueryTarget(target) {}

	EntityViewIterator EntityView::begin()
	{
		switch (m_QueryTarget)
		{
		case QueryTarget::AllEntities:
			return EntityViewIterator(m_Entities.GetEntityStorage(m_Archetype), 0);
		case QueryTarget::DeletedEntities:
			return EntityViewIterator(m_Entities.GetDeletedEntityStorage(m_Archetype), 0);
		}
	}

	EntityViewIterator EntityView::end()
	{
		switch (m_QueryTarget)
		{
		case QueryTarget::AllEntities:
		{
			EntityStorage& storage = m_Entities.GetEntityStorage(m_Archetype);
			return EntityViewIterator(storage, storage.GetEntityCount());
		}
		case QueryTarget::DeletedEntities:
		{
			EntityStorage& storage = m_Entities.GetDeletedEntityStorage(m_Archetype);
			return EntityViewIterator(storage, storage.GetEntityCount());
		}
		default:
			FLARE_CORE_ASSERT(false);
		}
	}

	std::optional<Entity> EntityView::GetEntity(size_t index)
	{
		switch (m_QueryTarget)
		{
		case QueryTarget::AllEntities:
		{
			const EntityStorage& storage = m_Entities.GetEntityStorage(m_Archetype);
			if (index >= storage.GetEntityCount())
				return {};

			return storage.GetEntityId(index);
		}
		case QueryTarget::DeletedEntities:
		{
			const EntityStorage& storage = m_Entities.GetDeletedEntityStorage(m_Archetype);
			if (index >= storage.GetEntityCount())
				return {};
			return storage.GetEntityId(index);
		}
		}

		return {};
	}

	ArchetypeId EntityView::GetArchetype() const
	{
		return m_Archetype;
	}
}