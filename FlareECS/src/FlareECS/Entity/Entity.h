#pragma once

#include "FlareECS/EntityId.h"
#include "FlareECS/Entity/Archetype.h"

#include <stdint.h>
#include <xhash>

namespace Flare
{
	struct EntityRecord
	{
		Entity Id;

		uint32_t RegistryIndex;
		ArchetypeId Archetype;
		size_t BufferIndex;
	};
}
