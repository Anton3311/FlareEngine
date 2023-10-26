#pragma once

#include "FlareCore/Serialization/TypeInitializer.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
	void SerializeType(YAML::Emitter& emitter, const TypeInitializer& type, const uint8_t* data);
	void DeserializeType(YAML::Node& node, const TypeInitializer& type, uint8_t* data);

	template<typename T>
	void SerializeType(YAML::Emitter& emitter, T& value)
	{
		SerializeType(emitter, T::_Type, (uint8_t*)&value);
	}

	template<typename T>
	void DeserializeType(YAML::Node& node, T& value)
	{
		DeserializeType(node, T::_Type, (uint8_t*)&value);
	}
}