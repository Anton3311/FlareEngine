#pragma once

#include "FlareCore/UUID.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/ComponentInitializer.h"

namespace Flare
{
	struct SerializationId
	{
		FLARE_COMPONENT;

		SerializationId() = default;
		SerializationId(UUID id)
			: Id(id) {}

		UUID Id;
	};
}