#pragma once

#include "FlareCore/Assert.h"

#include <string>
#include <vector>
#include <optional>

namespace Flare
{
    enum class SerializablePropertyType : uint8_t
    {
        Bool,

        Float,
        Int,
        UInt,
        FloatVector2,
        FloatVector3,
        FloatVector4,
        IntVector2,
        IntVector3,
        IntVector4,

        Matrix4x4,

        Color,
        Texture2D,

        Array,
        Object,
    };

    class SerializableObjectDescriptor;

    class SerializablePropertyDescriptor
    {
    public:
        SerializablePropertyDescriptor(std::string_view name, size_t offset, SerializablePropertyType type)
            : Name(name),
            Offset(offset),
            PropertyType(type),
            ObjectType(nullptr),
            ElementType({}),
            ArraySize(0) {}

        SerializablePropertyDescriptor(std::string_view name, size_t offset, SerializablePropertyType elementType, size_t arraySize)
            : Name(name),
            Offset(offset),
            PropertyType(SerializablePropertyType::Array),
            ObjectType(nullptr),
            ElementType(elementType),
            ArraySize(arraySize) {}

        SerializablePropertyDescriptor(std::string_view name, size_t offset, const SerializableObjectDescriptor* elementType, size_t arraySize)
            : Name(name),
            Offset(offset),
            PropertyType(SerializablePropertyType::Array),
            ObjectType(elementType),
            ElementType(SerializablePropertyType::Object),
            ArraySize(arraySize) {}
    public:
        std::string Name;
        size_t Offset;
        SerializablePropertyType PropertyType;
        size_t ArraySize;

        std::optional<SerializablePropertyType> ElementType;
        const SerializableObjectDescriptor* ObjectType;
    };

    class SerializableObjectDescriptor
    {
    public:
        std::string Name;
        std::vector<SerializablePropertyDescriptor> Properties;
    };

    class SerializableProperty
    {
    public:
        SerializableProperty(const uint8_t* buffer, const SerializablePropertyDescriptor& descriptor)
            : Buffer(buffer), Descriptor(descriptor) {}

        template<typename T>
        T& ValueAs()
        {
            return *(T*)Buffer;
        }
    public:
        const uint8_t* Buffer;
        const SerializablePropertyDescriptor& Descriptor;
    };

    class SerializableObject
    {
    public:
        SerializableObject(const uint8_t* buffer, const SerializableObjectDescriptor& descriptor)
            : Buffer(buffer), Descriptor(descriptor) {}

        inline SerializableProperty GetProperty(size_t index)
        {
            FLARE_CORE_ASSERT(index < Descriptor.Properties.size());
            const SerializablePropertyDescriptor& property = Descriptor.Properties[index];
            return SerializableProperty(Buffer + property.Offset, property);
        }
    public:
        const uint8_t* Buffer;
        const SerializableObjectDescriptor& Descriptor;
    };
}