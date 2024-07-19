#include "EntityIndex.h"

namespace Flare
{
	EntityIndex::EntityIndex(size_t reservedStackSize)
	{
		m_DeletedIds.reserve(reservedStackSize);
	}

	Entity EntityIndex::CreateId()
	{
		if (m_DeletedIds.size() == 0)
			return Entity(m_EntityNextIndex++, 0);

		Entity id = m_DeletedIds.back();
		m_DeletedIds.erase(m_DeletedIds.end() - 1);

		return id;
	}

	void EntityIndex::AddDeletedId(Entity entity)
	{
		m_DeletedIds.emplace_back(entity.GetIndex(), entity.GetGeneration() + 1ui16);
	}
}