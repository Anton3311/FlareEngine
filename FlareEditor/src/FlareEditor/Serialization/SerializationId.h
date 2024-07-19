#pragma once

#include "FlareCore/UUID.h"
#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"

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

	template<>
	struct TypeSerializer<SerializationId>
	{
		void OnSerialize(SerializationId& id, SerializationStream& stream)
		{
			stream.Serialize("Id", SerializationValue(id.Id));
		}
	};
}