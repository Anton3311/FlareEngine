#pragma once

#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
	void SerializeObject(YAML::Emitter& emitter, const SerializableObject& object);
	void DeserializeObject(YAML::Node& node, SerializableObject& object);
}