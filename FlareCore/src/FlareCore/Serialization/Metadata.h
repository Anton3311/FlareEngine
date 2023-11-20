#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Serialization/Serialization.h"

#include <glm/glm.hpp>

namespace Flare
{
    template<typename T>
    struct SerializablePropertyDataFromType
    {
        constexpr SerializablePropertyDescriptor::InitializationData Get()
        {
            return SerializablePropertyDescriptor::InitializationData(
                SerializablePropertyType::Object,
                0, SerializablePropertyType::None, 
                &FLARE_SERIALIZATION_DESCRIPTOR_OF(T));
        }
    };

#define TYPE_TO_PROPERTY_TYPE(typeName, propertyType) template<>              \
    struct SerializablePropertyDataFromType<typeName>                         \
    {                                                                         \
        constexpr SerializablePropertyDescriptor::InitializationData Get()    \
        {                                                                     \
            return SerializablePropertyDescriptor::InitializationData(        \
                propertyType,                                                 \
                0, SerializablePropertyType::None,                            \
                nullptr);                                                     \
        }                                                                     \
    };

    TYPE_TO_PROPERTY_TYPE(bool, Flare::SerializablePropertyType::Bool);
    TYPE_TO_PROPERTY_TYPE(float, Flare::SerializablePropertyType::Float);

    TYPE_TO_PROPERTY_TYPE(int8_t, Flare::SerializablePropertyType::Int8);
    TYPE_TO_PROPERTY_TYPE(uint8_t, Flare::SerializablePropertyType::UInt8);
    TYPE_TO_PROPERTY_TYPE(int16_t, Flare::SerializablePropertyType::Int16);
    TYPE_TO_PROPERTY_TYPE(uint16_t, Flare::SerializablePropertyType::UInt16);
    TYPE_TO_PROPERTY_TYPE(int32_t, Flare::SerializablePropertyType::Int32);
    TYPE_TO_PROPERTY_TYPE(uint32_t, Flare::SerializablePropertyType::UInt32);
    TYPE_TO_PROPERTY_TYPE(int64_t, Flare::SerializablePropertyType::Int64);
    TYPE_TO_PROPERTY_TYPE(uint64_t, Flare::SerializablePropertyType::UInt64);

    TYPE_TO_PROPERTY_TYPE(glm::vec2, Flare::SerializablePropertyType::FloatVector2);
    TYPE_TO_PROPERTY_TYPE(glm::vec3, Flare::SerializablePropertyType::FloatVector3);
    TYPE_TO_PROPERTY_TYPE(glm::vec4, Flare::SerializablePropertyType::FloatVector4);

    TYPE_TO_PROPERTY_TYPE(glm::ivec2, Flare::SerializablePropertyType::IntVector2);
    TYPE_TO_PROPERTY_TYPE(glm::ivec3, Flare::SerializablePropertyType::IntVector3);
    TYPE_TO_PROPERTY_TYPE(glm::ivec4, Flare::SerializablePropertyType::IntVector4);

    TYPE_TO_PROPERTY_TYPE(glm::uvec2, Flare::SerializablePropertyType::UIntVector2);
    TYPE_TO_PROPERTY_TYPE(glm::uvec3, Flare::SerializablePropertyType::UIntVector3);
    TYPE_TO_PROPERTY_TYPE(glm::uvec4, Flare::SerializablePropertyType::UIntVector4);
    
    TYPE_TO_PROPERTY_TYPE(std::string, Flare::SerializablePropertyType::String);

#undef TYPE_TO_PROPERTY_TYPE

    template<typename T, size_t N>
    struct SerializablePropertyDataFromType<T[N]>
    {
        constexpr SerializablePropertyDescriptor::InitializationData Get()
        {
            SerializablePropertyDescriptor::InitializationData elementInfo = SerializablePropertyDataFromType<T>().Get();
            return SerializablePropertyDescriptor::InitializationData(SerializablePropertyType::Array, N, elementInfo.Type, elementInfo.ObjectType);
        }
    };

#define FLARE_SERIALIZABLE \
    static Flare::SerializableObjectDescriptor _SerializationDescriptor;

#define FLARE_SERIALIZABLE_IMPL(typeName, ...) \
    Flare::SerializableObjectDescriptor typeName::_SerializationDescriptor( \
        typeid(typeName).name(), sizeof(typeName), { __VA_ARGS__ });

#define FLARE_PROPERTY(typeName, propertyName)                                                        \
    Flare::SerializablePropertyDescriptor(                                                            \
        #propertyName, offsetof(typeName, propertyName),                                              \
        Flare::SerializablePropertyDataFromType<FLARE_TYPE_OF_FIELD(typeName, propertyName)>().Get())

#define FLARE_ENUM_PROPERTY(typeName, propertyName)                                                                           \
    Flare::SerializablePropertyDescriptor(                                                                                    \
        #propertyName, offsetof(typeName, propertyName),                                                                      \
        Flare::SerializablePropertyDataFromType<std::underlying_type_t<FLARE_TYPE_OF_FIELD(typeName, propertyName)>>().Get())
}