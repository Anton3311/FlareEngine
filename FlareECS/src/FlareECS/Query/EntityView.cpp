#include "EntityView.h"

namespace Flare
{
	EntityView::EntityView(Registry& registry, ArchetypeId archetype)
		: m_Registry(registry), m_Archetype(archetype) {}

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
		if (index >= indices.size())
			return {};

		return m_Registry.FindEntityByRegistryIndex(indices[index]);
	}

	ArchetypeId EntityView::GetArchetype() const
	{
		return m_Archetype;
	}
}