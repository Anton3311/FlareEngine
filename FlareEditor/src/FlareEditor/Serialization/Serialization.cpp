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
		case SerializablePropertyType::Int8:
			emitter << property.ValueAs<int8_t>();
			break;
		case SerializablePropertyType::UInt8:
			emitter << property.ValueAs<uint8_t>();
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
			emitter << property.ValueAs<std::string>();
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
			property.SetValue<int8_t>(propertyNode.as<int8_t>());
			break;
		case SerializablePropertyType::UInt8:
			property.SetValue<uint8_t>(propertyNode.as<uint8_t>());
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

	static void SerializeField(YAML::Emitter& emitter, const FieldTypeInfo& typeInfo, const uint8_t* data)
	{
		switch (typeInfo.FieldType)
		{
		case SerializableFieldType::Bool:
			emitter << YAML::Value << *(bool*)(data);
			break;

		case SerializableFieldType::Int8:
			emitter << YAML::Value << (int16_t) *(int8_t*)(data);
			break;
		case SerializableFieldType::UInt8:
			emitter << YAML::Value << (uint16_t) *(uint8_t*)(data);
			break;

		case SerializableFieldType::Int16:
			emitter << YAML::Value << *(int16_t*)(data);
			break;
		case SerializableFieldType::UInt16:
			emitter << YAML::Value << *(uint16_t*)(data);
			break;

		case SerializableFieldType::Int32:
			emitter << YAML::Value << *(int32_t*)(data);
			break;
		case SerializableFieldType::UInt32:
			emitter << YAML::Value << *(uint32_t*)(data);
			break;

		case SerializableFieldType::Int2:
			emitter << YAML::Value << *(glm::ivec2*)(data);
			break;
		case SerializableFieldType::Int3:
			emitter << YAML::Value << *(glm::ivec3*)(data);
			break;
		case SerializableFieldType::Int4:
			emitter << YAML::Value << *(glm::ivec4*)(data);
			break;

		case SerializableFieldType::Float32:
			emitter << YAML::Value << *(float*)(data);
			break;
		case SerializableFieldType::Float2:
			emitter << YAML::Value << *(glm::vec2*)(data);
			break;
		case SerializableFieldType::Float3:
			emitter << YAML::Value << *(glm::vec3*)(data);
			break;
		case SerializableFieldType::Float4:
			emitter << YAML::Value << *(glm::vec4*)(data);
			break;
		case SerializableFieldType::String:
		{
			std::string copy = *(std::string*)data;
			emitter << YAML::Value << copy;
			break;
		}
		case SerializableFieldType::Custom:
		{
			FLARE_CORE_ASSERT(typeInfo.CustomType);
			if (typeInfo.CustomType == &Entity::_Type)
			{
				FLARE_CORE_WARN("Entity serialization is not supported yet");
				emitter << YAML::Value << UINT64_MAX;
				break;
			}
			else if (typeInfo.CustomType == &AssetHandle::_Type)
			{
				emitter << YAML::Value << *(AssetHandle*)data;
				break;
			}

			emitter << YAML::Value;
			SerializeType(emitter, *typeInfo.CustomType, data);
			break;
		}
		default:
			FLARE_CORE_ASSERT(false, "Unhandled field type");
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

	void SerializeType(YAML::Emitter& emitter, const TypeInitializer& type, const uint8_t* data)
	{
#if 0
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Name" << YAML::Value << std::string(type.TypeName);

		for (const FieldData& field : type.SerializedFields)
		{
			const uint8_t* fieldData = (const uint8_t*)data + field.Offset;
			switch (field.TypeInfo.FieldType)
			{
			case SerializableFieldType::Array:
			{
				emitter << YAML::Key << field.Name << YAML::Value << YAML::BeginSeq;

				size_t elementSize = field.ArrayElementType.GetSize();
				for (size_t i = 0; i < field.ElementsCount; i++)
					SerializeField(emitter, field.ArrayElementType, fieldData + i * elementSize);

				emitter << YAML::EndSeq;
				break;
			}
			default:
			{
				emitter << YAML::Key << field.Name;
				SerializeField(emitter, field.TypeInfo, fieldData);
			}
			}
		}

		emitter << YAML::EndMap;
#endif
	}

	static void DeserializeField(YAML::Node& fieldNode, const FieldTypeInfo& fieldType, uint8_t* data)
	{
#if 0
		switch (fieldType.FieldType)
		{
		case SerializableFieldType::Bool:
		{
			bool value = fieldNode.as<bool>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::Int8:
		{
			int8_t value = (int8_t)fieldNode.as<int16_t>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::UInt8:
		{
			int8_t value = (uint8_t)fieldNode.as<uint16_t>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}

		case SerializableFieldType::Int16:
		{
			int16_t value = fieldNode.as<int16_t>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::UInt16:
		{
			int16_t value = fieldNode.as<uint16_t>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}

		case SerializableFieldType::Int32:
		{
			int32_t value = fieldNode.as<int32_t>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::UInt32:
		{
			int32_t value = fieldNode.as<uint32_t>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}

		case SerializableFieldType::Int2:
		{
			glm::ivec2 value = fieldNode.as<glm::ivec2>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::Int3:
		{
			glm::ivec3 value = fieldNode.as<glm::ivec3>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::Int4:
		{
			glm::ivec4 value = fieldNode.as<glm::ivec4>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}

		case SerializableFieldType::Float32:
		{
			float value = fieldNode.as<float>();
			std::memcpy(data, &value, sizeof(value));
			break;
		}
		case SerializableFieldType::Float2:
		{
			glm::vec2 vector = fieldNode.as<glm::vec2>();
			std::memcpy(data, &vector, sizeof(vector));
			break;
		}
		case SerializableFieldType::Float3:
		{
			glm::vec3 vector = fieldNode.as<glm::vec3>();
			std::memcpy(data, &vector, sizeof(vector));
			break;
		}
		case SerializableFieldType::Float4:
		{
			glm::vec4 vector = fieldNode.as<glm::vec4>();
			std::memcpy(data, &vector, sizeof(vector));
			break;
		}
		case SerializableFieldType::String:
		{
			std::string string = fieldNode.as<std::string>();

			std::string* destString = (std::string*)data;
			new (destString) std::string();

			*destString = std::move(string);
			break;
		}
		case SerializableFieldType::Custom:
		{
			FLARE_CORE_ASSERT(fieldType.CustomType);
			if (fieldType.CustomType == &Entity::_Type)
			{
				FLARE_CORE_WARN("Entity deserialization is not supported yet");
				break;
			}
			else if (fieldType.CustomType == &AssetHandle::_Type)
			{
				AssetHandle handle = fieldNode.as<AssetHandle>();
				std::memcpy(data, &handle, sizeof(handle));
				break;
			}

			DeserializeType(fieldNode, *fieldType.CustomType, data);
			break;
		}
		default:
			FLARE_CORE_ASSERT(false, "Unhandled field type");
		}
#endif
	}

	void DeserializeType(YAML::Node& node, const TypeInitializer& type, uint8_t* data)
	{
#if 0
		for (const FieldData& field : type.SerializedFields)
		{
			if (YAML::Node fieldNode = node[field.Name])
			{
				switch (field.TypeInfo.FieldType)
				{
				case SerializableFieldType::Array:
				{
					size_t elementSize = field.ArrayElementType.GetSize();
					size_t index = 0;
					for (YAML::Node element : fieldNode)
					{
						DeserializeField(element, field.ArrayElementType, data + field.Offset + index * elementSize);
						index++;

						if (index >= field.ElementsCount)
							break;
					}
					break;
				}
				default:
				{
					DeserializeField(fieldNode, field.TypeInfo, data + field.Offset);
				}
				}
			}
		}
#endif
	}
}