#include "Serialization.h"

#include "Flare/AssetManager/Asset.h"
#include "Flare/Serialization/Serialization.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	static void SerializeProperty(YAML::Emitter& emitter, const SerializableProperty& property)
	{
		switch (property.Descriptor.PropertyType)
		{
		case SerializablePropertyType::None:
			break;
		case SerializablePropertyType::Bool:
			emitter << property.ValueAs<bool>();
			break;
		case SerializablePropertyType::Float:
			emitter << property.ValueAs<float>();
			break;

			// Serialize int8_t and uint8_t as int16, because int8_t and uint8_t are serialized as chars
		case SerializablePropertyType::Int8:
			emitter << (int16_t)property.ValueAs<int8_t>();
			break;
		case SerializablePropertyType::UInt8:
			emitter << (uint16_t)property.ValueAs<uint8_t>();
			break;
		case SerializablePropertyType::Int16:
			emitter << property.ValueAs<int16_t>();
			break;
		case SerializablePropertyType::UInt16:
			emitter << property.ValueAs<uint16_t>();
			break;
		case SerializablePropertyType::Int32:
			emitter << property.ValueAs<int32_t>();
			break;
		case SerializablePropertyType::UInt32:
			emitter << property.ValueAs<uint32_t>();
			break;
		case SerializablePropertyType::Int64:
			emitter << property.ValueAs<int64_t>();
			break;
		case SerializablePropertyType::UInt64:
			emitter << property.ValueAs<uint64_t>();
			break;
		case SerializablePropertyType::FloatVector2:
			emitter << property.ValueAs<glm::vec2>();
			break;
		case SerializablePropertyType::FloatVector3:
			emitter << property.ValueAs<glm::vec3>();
			break;
		case SerializablePropertyType::FloatVector4:
			emitter << property.ValueAs<glm::vec4>();
			break;
		case SerializablePropertyType::IntVector2:
			emitter << property.ValueAs<glm::ivec2>();
			break;
		case SerializablePropertyType::IntVector3:
			emitter << property.ValueAs<glm::ivec3>();
			break;
		case SerializablePropertyType::IntVector4:
			emitter << property.ValueAs<glm::ivec4>();
			break;
		case SerializablePropertyType::String:
		{
			std::string stringCopy = property.ValueAs<std::string>();
			emitter << stringCopy;
			break;
		}
		case SerializablePropertyType::UUID:
			emitter << property.ValueAs<UUID>();
			break;
		case SerializablePropertyType::Color:
			emitter << property.ValueAs<glm::vec4>();
			break;
		case SerializablePropertyType::Object:
		{
			FLARE_CORE_ASSERT(property.Descriptor.ObjectType);
			if (property.Descriptor.ObjectType == &FLARE_SERIALIZATION_DESCRIPTOR_OF(Entity))
			{
				FLARE_CORE_WARN("Entity serialization is not supported yet");
				emitter << YAML::Value << UINT64_MAX;
				break;
			}
			else if (property.Descriptor.ObjectType == &FLARE_SERIALIZATION_DESCRIPTOR_OF(AssetHandle))
			{
				emitter << YAML::Value << property.ValueAs<AssetHandle>();
				break;
			}

			emitter << YAML::Value;

			SerializeObject(emitter, property.AsObject());
			break;
		}
		}
	}

	static void DeserializeProperty(YAML::Node& propertyNode, SerializableProperty& property)
	{
		switch (property.Descriptor.PropertyType)
		{
		case SerializablePropertyType::None:
			break;
		case SerializablePropertyType::Bool:
			property.SetValue<bool>(propertyNode.as<bool>());
			break;
		case SerializablePropertyType::Float:
			property.SetValue<float>(propertyNode.as<float>());
			break;
		case SerializablePropertyType::Int8:
			property.SetValue<int8_t>((int8_t)propertyNode.as<int16_t>());
			break;
		case SerializablePropertyType::UInt8:
			property.SetValue<uint8_t>((uint8_t)propertyNode.as<uint16_t>());
			break;
		case SerializablePropertyType::Int16:
			property.SetValue<int16_t>(propertyNode.as<int16_t>());
			break;
		case SerializablePropertyType::UInt16:
			property.SetValue<uint16_t>(propertyNode.as<uint16_t>());
			break;
		case SerializablePropertyType::Int32:
			property.SetValue<int32_t>(propertyNode.as<int32_t>());
			break;
		case SerializablePropertyType::UInt32:
			property.SetValue<uint32_t>(propertyNode.as<uint32_t>());
			break;
		case SerializablePropertyType::Int64:
			property.SetValue<int64_t>(propertyNode.as<int64_t>());
			break;
		case SerializablePropertyType::UInt64:
			property.SetValue<uint64_t>(propertyNode.as<uint64_t>());
			break;
		case SerializablePropertyType::FloatVector2:
			property.SetValue<glm::vec2>(propertyNode.as<glm::vec2>());
			break;
		case SerializablePropertyType::FloatVector3:
			property.SetValue<glm::vec3>(propertyNode.as<glm::vec3>());
			break;
		case SerializablePropertyType::FloatVector4:
			property.SetValue<glm::vec4>(propertyNode.as<glm::vec4>());
			break;
		case SerializablePropertyType::IntVector2:
			property.SetValue<glm::ivec2>(propertyNode.as<glm::ivec2>());
			break;
		case SerializablePropertyType::IntVector3:
			property.SetValue<glm::ivec3>(propertyNode.as<glm::ivec3>());
			break;
		case SerializablePropertyType::IntVector4:
			property.SetValue<glm::ivec4>(propertyNode.as<glm::ivec4>());
			break;
		case SerializablePropertyType::String:
			property.SetValue<std::string>(propertyNode.as<std::string>());
			break;
		case SerializablePropertyType::Color:
			property.SetValue<glm::vec4>(propertyNode.as<glm::vec4>());
			break;
		case SerializablePropertyType::UUID:
			property.SetValue<UUID>(propertyNode.as<UUID>());
			break;
		case SerializablePropertyType::Object:
		{
			FLARE_CORE_ASSERT(property.Descriptor.ObjectType);
			if (property.Descriptor.ObjectType == &FLARE_SERIALIZATION_DESCRIPTOR_OF(Entity))
			{
				FLARE_CORE_WARN("Entity deserialization is not supported yet");
				break;
			}
			else if (property.Descriptor.ObjectType == &FLARE_SERIALIZATION_DESCRIPTOR_OF(AssetHandle))
			{
				property.SetValue<AssetHandle>(propertyNode.as<AssetHandle>());
				break;
			}

			SerializableObject object = property.AsObject();
			DeserializeObject(propertyNode, object);
			break;
		}
		}
	}

	void SerializeObject(YAML::Emitter& emitter, const SerializableObject& object)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Name" << YAML::Value << std::string(object.Descriptor.Name);

		for (size_t i = 0; i < object.Descriptor.Properties.size(); i++)
		{
			const SerializableProperty property = object.PropertyAt(i);

			switch (property.Descriptor.PropertyType)
			{
			case SerializablePropertyType::Array:
			{
				emitter << YAML::Key << property.Descriptor.Name << YAML::Value << YAML::BeginSeq;

				size_t elementSize = property.Descriptor.GetArrayElementSize();
				for (size_t elementIndex = 0; elementIndex < property.Descriptor.ArraySize; elementIndex++)
					SerializeProperty(emitter, property.GetArrayElement(elementIndex));

				emitter << YAML::EndSeq;
				break;
			}
			default:
			{
				emitter << YAML::Key << property.Descriptor.Name;
				SerializeProperty(emitter, property);
			}
			}
		}

		emitter << YAML::EndMap;
	}

	void DeserializeObject(YAML::Node& node, SerializableObject& object)
	{
		for (size_t propertyIndex = 0; propertyIndex < object.Descriptor.Properties.size(); propertyIndex++)
		{
			if (YAML::Node propertyNode = node[object.Descriptor.Properties[propertyIndex].Name])
			{
				SerializableProperty property = object.PropertyAt(propertyIndex);
				switch (property.Descriptor.PropertyType)
				{
				case SerializablePropertyType::Array:
				{
					size_t elementSize = property.Descriptor.GetArrayElementSize();
					size_t index = 0;
					for (YAML::Node element : propertyNode)
					{
						SerializableProperty elementProperty = property.GetArrayElement(index);
						DeserializeProperty(element, elementProperty);
						index++;

						if (index >= property.Descriptor.ArraySize)
							break;
					}
					break;
				}
				default:
					DeserializeProperty(propertyNode, property);
				}
			}
		}
	}
}