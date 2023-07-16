#include "EntityView.h"

namespace Flare
{
	EntityViewIterator EntityView::begin()
	{
		return EntityViewIterator(m_Registry.GetArchetypeRecord(m_Archetype).Storage, 0);
	}

	EntityViewIterator EntityView::end()
	{
		EntityStorage& storage = m_Registry.GetArchetypeRecord(m_Archetype).Storage;
		return EntityViewIterator(storage, storage.GetEntitiesCount());
	}

	std::optional<Entity> EntityView::GetEntity(size_t index)
	{
		const auto& indices = m_Registry.GetArchetypeRecord(m_Archetype).Storage.GetEntityIndices();
		FLARE_CORE_ASSERT(index < indices.size());

		return m_Registry.FindEntityByRegistryIndex(indices[index]);
	}
}