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
			return EntityViewIterator(m_Entities.GetEntityStorage(m_Archetype).GetDataStorage(), 0);
		case QueryTarget::DeletedEntities:
			return EntityViewIterator(m_Entities.GetDeletedEntityStorage(m_Archetype).DataStorage, 0);
		}
	}

	EntityViewIterator EntityView::end()
	{
		switch (m_QueryTarget)
		{
		case QueryTarget::AllEntities:
		{
			EntityStorage& storage = m_Entities.GetEntityStorage(m_Archetype);
			return EntityViewIterator(storage.GetDataStorage(), storage.GetEntitiesCount());
		}
		case QueryTarget::DeletedEntities:
		{
			EntityDataStorage& storage = m_Entities.GetDeletedEntityStorage(m_Archetype).DataStorage;
			return EntityViewIterator(storage, storage.EntitiesCount);
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
			const auto& indices = m_Entities.GetEntityStorage(m_Archetype).GetEntityIndices();
			if (index >= indices.size())
				return {};

			return m_Entities.FindEntityByRegistryIndex(indices[index]);
		}
		case QueryTarget::DeletedEntities:
		{
			const DeletedEntitiesStorage& storage = m_Entities.GetDeletedEntityStorage(m_Archetype);
			if (index >= storage.DataStorage.EntitiesCount)
				return {};
			return storage.Ids[index];
		}
		}

		return {};
	}

	ArchetypeId EntityView::GetArchetype() const
	{
		return m_Archetype;
	}
}