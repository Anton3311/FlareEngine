#pragma once

#include "FlareCore/Assert.h"

#include <string>
#include <vector>
#include <optional>

namespace Flare
{
#define FLARE_SERIALIZATION_DESCRIPTOR_OF(typeName) (typeName::_SerializationDescriptor)

    enum class SerializablePropertyType : uint8_t
    {
        None,

        Bool,

        Float,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,

        FloatVector2,
        FloatVector3,
        FloatVector4,

        IntVector2,
        IntVector3,
        IntVector4,

        UIntVector2,
        UIntVector3,
        UIntVector4,

        Matrix4x4,

        String,

        Color,
        Texture2D,

        Array,
        Object,
    };

    class SerializableObjectDescriptor;

    class FLARECORE_API SerializablePropertyDescriptor
    {
    public:
        struct InitializationData
        {
            constexpr InitializationData(SerializablePropertyType type, size_t arraySize,
                SerializablePropertyType elementType, const SerializableObjectDescriptor* objectType)
                : Type(type), ArraySize(arraySize),
                ElementType(elementType), ObjectType(objectType) {}

            SerializablePropertyType Type;
            size_t ArraySize;
            SerializablePropertyType ElementType;
            const SerializableObjectDescriptor* ObjectType;
        };

        SerializablePropertyDescriptor(std::string_view name, size_t offset, const InitializationData& data);
        SerializablePropertyDescriptor(std::string_view name, size_t offset, SerializablePropertyType type);
        SerializablePropertyDescriptor(std::string_view name, size_t offset, SerializablePropertyType elementType, size_t arraySize);
        SerializablePropertyDescriptor(std::string_view name, size_t offset, const SerializableObjectDescriptor* elementType, size_t arraySize);
        
        size_t GetArrayElementSize() const;
    public:
        std::string Name;
        size_t Offset;
        SerializablePropertyType PropertyType;
        size_t ArraySize;

        SerializablePropertyType ElementType;
        const SerializableObjectDescriptor* ObjectType;
    };

    class SerializableObjectDescriptor
    {
    public:
        SerializableObjectDescriptor() = default;
        SerializableObjectDescriptor(std::string_view name, size_t size, const std::initializer_list<SerializablePropertyDescriptor>& properties)
            : Name(name), Size(size), Properties(properties) {}
    public:
        std::string Name;
        size_t Size;
        std::vector<SerializablePropertyDescriptor> Properties;
    };

    class SerializableObject;
    class FLARECORE_API SerializableProperty
    {
    public:
        SerializableProperty(uint8_t* buffer, const SerializablePropertyDescriptor& descriptor)
            : m_Buffer(buffer), Descriptor(descriptor) {}

        template<typename T>
        T& ValueAs()
        {
            return *(T*)m_Buffer;
        }

        template<typename T>
        void SetValue(const T& value)
        {
            T* dest = new(m_Buffer) T();
            *dest = value;
        }

        template<typename T>
        const T& ValueAs() const
        {
            return *(T*)m_Buffer;
        }

        SerializableObject AsObject() const;

        const SerializableProperty GetArrayElement(size_t index) const
        {
            FLARE_CORE_ASSERT(index < Descriptor.ArraySize);
            size_t elementSize = Descriptor.GetArrayElementSize();
            return SerializableProperty(m_Buffer + elementSize, Descriptor);
        }

        const uint8_t* GetBuffer() const { return m_Buffer; }
    public:
        const SerializablePropertyDescriptor& Descriptor;
    private:
        uint8_t* m_Buffer;
    };

    class SerializableObject
    {
    public:
        SerializableObject(uint8_t* buffer, const SerializableObjectDescriptor& descriptor)
            : m_Buffer(buffer), Descriptor(descriptor) {}

        template<typename T>
        inline static SerializableObject From(T& value)
        {
            return SerializableObject((uint8_t*)&value, FLARE_SERIALIZATION_DESCRIPTOR_OF(T));
        }

        inline SerializableProperty PropertyAt(size_t index)
        {
            FLARE_CORE_ASSERT(index < Descriptor.Properties.size());
            const SerializablePropertyDescriptor& property = Descriptor.Properties[index];
            return SerializableProperty(m_Buffer + property.Offset, property);
        }

        inline const SerializableProperty PropertyAt(size_t index) const
        {
            FLARE_CORE_ASSERT(index < Descriptor.Properties.size());
            const SerializablePropertyDescriptor& property = Descriptor.Properties[index];
            return SerializableProperty(m_Buffer + property.Offset, property);
        }
    public:
        const SerializableObjectDescriptor& Descriptor;
    private:
        uint8_t* m_Buffer;
    };
}