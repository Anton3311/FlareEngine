#include "Serialization.h"

#include "Flare/AssetManager/Asset.h"
#include "Flare/Serialization/Serialization.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
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

	void SerializeType(YAML::Emitter& emitter, const TypeInitializer& type, const uint8_t* data)
	{
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
	}

	static void DeserializeField(YAML::Node& fieldNode, const FieldTypeInfo& fieldType, uint8_t* data)
	{
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
	}

	void DeserializeType(YAML::Node& node, const TypeInitializer& type, uint8_t* data)
	{
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
	}
}