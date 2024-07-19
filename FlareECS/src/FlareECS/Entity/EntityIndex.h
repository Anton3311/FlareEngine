#pragma once

#include "FlareECS/Entity/Entity.h"

#include <stdint.h>
#include <vector>

namespace Flare
{
	class EntityIndex
	{
	public:
		EntityIndex(size_t reservedStackSize = 64);
	public:
		Entity CreateId();

		// Inserts an id into stack and increaments its generation count
		void AddDeletedId(Entity entity);
	private:
		uint32_t m_EntityNextIndex = 0;
		std::vector<Entity> m_DeletedIds;
	};
}