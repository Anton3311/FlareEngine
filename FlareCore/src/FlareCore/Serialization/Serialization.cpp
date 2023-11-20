#include "Serialization.h"

#include <glm/glm.hpp>

namespace Flare
{
    SerializablePropertyDescriptor::SerializablePropertyDescriptor(std::string_view name, size_t offset, const InitializationData& data)
        : Name(name), Offset(offset), PropertyType(data.Type), ArraySize(data.ArraySize),
        ElementType(data.ElementType), ObjectType(data.ObjectType) {}

    SerializablePropertyDescriptor::SerializablePropertyDescriptor(std::string_view name, size_t offset, SerializablePropertyType type)
        : Name(name),
        Offset(offset),
        PropertyType(type),
        ObjectType(nullptr),
        ElementType(SerializablePropertyType::None),
        ArraySize(0) {}

    SerializablePropertyDescriptor::SerializablePropertyDescriptor(std::string_view name, size_t offset, SerializablePropertyType elementType, size_t arraySize)
        : Name(name),
        Offset(offset),
        PropertyType(SerializablePropertyType::Array),
        ObjectType(nullptr),
        ElementType(elementType),
        ArraySize(arraySize) {}

    SerializablePropertyDescriptor::SerializablePropertyDescriptor(std::string_view name, size_t offset, const SerializableObjectDescriptor* elementType, size_t arraySize)
        : Name(name),
        Offset(offset),
        PropertyType(SerializablePropertyType::Array),
        ObjectType(elementType),
        ElementType(SerializablePropertyType::Object),
        ArraySize(arraySize) {}

    size_t SerializablePropertyDescriptor::GetArrayElementSize() const
    {
        switch (ElementType)
        {
        case SerializablePropertyType::None:
            return 0;
        case SerializablePropertyType::Bool:
            return sizeof(bool);
        case SerializablePropertyType::Float:
            return sizeof(float);
        case SerializablePropertyType::Int8:
            return sizeof(int8_t);
        case SerializablePropertyType::UInt8:
            return sizeof(uint8_t);
        case SerializablePropertyType::Int16:
            return sizeof(int16_t);
        case SerializablePropertyType::UInt16:
            return sizeof(uint16_t);
        case SerializablePropertyType::Int32:
            return sizeof(int32_t);
        case SerializablePropertyType::UInt32:
            return sizeof(uint32_t);
        case SerializablePropertyType::Int64:
            return sizeof(int64_t);
        case SerializablePropertyType::UInt64:
            return sizeof(uint64_t);
        case SerializablePropertyType::FloatVector2:
            return sizeof(glm::vec2);
        case SerializablePropertyType::FloatVector3:
            return sizeof(glm::vec3);
        case SerializablePropertyType::FloatVector4:
            return sizeof(glm::vec4);
        case SerializablePropertyType::IntVector2:
            return sizeof(glm::ivec2);
        case SerializablePropertyType::IntVector3:
            return sizeof(glm::ivec3);
        case SerializablePropertyType::IntVector4:
            return sizeof(glm::ivec4);
        case SerializablePropertyType::UIntVector2:
            return sizeof(glm::uvec2);
        case SerializablePropertyType::UIntVector3:
            return sizeof(glm::uvec3);
        case SerializablePropertyType::UIntVector4:
            return sizeof(glm::uvec4);
        case SerializablePropertyType::Matrix4x4:
            return sizeof(glm::mat4);
        case SerializablePropertyType::String:
            return sizeof(std::string);
        case SerializablePropertyType::Color:
            return sizeof(glm::vec4);
        case SerializablePropertyType::Texture2D:
            return 0;
        case SerializablePropertyType::Array:
            return 0;
        case SerializablePropertyType::Object:
            FLARE_CORE_ASSERT(ObjectType);
            return 0;
        }
        return 0;
    }

    SerializableObject SerializableProperty::AsObject() const
    {
        FLARE_CORE_ASSERT(Descriptor.PropertyType == SerializablePropertyType::Object);
        FLARE_CORE_ASSERT(Descriptor.ObjectType);
        return SerializableObject(m_Buffer, *Descriptor.ObjectType);
    }
}
