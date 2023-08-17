#pragma once

#include "Flare/Core/Core.h"

#include <glm/glm.hpp>

#include <vector>
#include <string_view>

namespace Flare
{
    enum class SerializableFieldType : uint8_t
    {
        None,

        Bool,

        Int8,
        UInt8,

        Int16,
        UInt16,

        Int32,
        UInt32,

        Int64,
        UInt64,

        Float32,
        Float64,

        Float2,
        Float3,
        Float4,

        Int2,
        Int3,
        Int4,

        UInt2,
        UInt3,
        UInt4,
    };

    struct FieldData
    {
        const char* Name;
        SerializableFieldType Type;
        size_t Offset;
    };

    template<typename T>
    constexpr SerializableFieldType GetFieldType()
    {
        if constexpr (std::is_same<T, float>().value)
            return SerializableFieldType::Float32;
        if constexpr (std::is_same<T, double>().value)
            return SerializableFieldType::Float64;

        return SerializableFieldType::None;
    }

#define GET_FIELD_TYPE(typeName, fieldType) template<> constexpr SerializableFieldType GetFieldType<typeName>() { return fieldType; }

    GET_FIELD_TYPE(bool, SerializableFieldType::Bool);

    GET_FIELD_TYPE(int8_t,   SerializableFieldType::Int8);
    GET_FIELD_TYPE(uint8_t,  SerializableFieldType::UInt8);
    GET_FIELD_TYPE(int16_t,  SerializableFieldType::Int16);
    GET_FIELD_TYPE(uint16_t, SerializableFieldType::UInt16);
    GET_FIELD_TYPE(int32_t,  SerializableFieldType::Int32);
    GET_FIELD_TYPE(uint32_t, SerializableFieldType::UInt32);
    GET_FIELD_TYPE(int64_t,  SerializableFieldType::Int64);
    GET_FIELD_TYPE(uint64_t, SerializableFieldType::UInt64);

    GET_FIELD_TYPE(glm::vec2, SerializableFieldType::Float2);
    GET_FIELD_TYPE(glm::vec3, SerializableFieldType::Float3);
    GET_FIELD_TYPE(glm::vec4, SerializableFieldType::Float4);

    GET_FIELD_TYPE(glm::ivec2, SerializableFieldType::Int2);
    GET_FIELD_TYPE(glm::ivec3, SerializableFieldType::Int3);
    GET_FIELD_TYPE(glm::ivec4, SerializableFieldType::Int4);

    GET_FIELD_TYPE(glm::uvec2, SerializableFieldType::UInt2);
    GET_FIELD_TYPE(glm::uvec3, SerializableFieldType::UInt3);
    GET_FIELD_TYPE(glm::uvec4, SerializableFieldType::UInt4);

#undef GET_FIELD_TYPE

#define FLARE_GET_FIELD_TYPE(typeName, fieldName) decltype(((typeName*)nullptr)->fieldName)
#define FLARE_FIELD(typeName, fieldName) FieldData{#fieldName, GetFieldType<FLARE_GET_FIELD_TYPE(typeName, fieldName)>(), offsetof(typeName, fieldName)}

    class FLARECOMMON_API TypeInitializer
    {
    public:
        using DefaultConstructorFunction = void(*)(void*);
        using DestructorFunction = void(*)(void*);

        TypeInitializer(std::string_view typeName, size_t size, 
            DestructorFunction destructor, 
            DefaultConstructorFunction constructor, 
            const std::initializer_list<FieldData>& fields);
        ~TypeInitializer();

        static std::vector<TypeInitializer*>& GetInitializers();
    public:
        const std::string_view TypeName;
        const std::vector<FieldData> SerializedFields;
        const DestructorFunction Destructor;
        const DefaultConstructorFunction DefaultConstructor;
        const size_t Size;
    };
}

#define FLARE_TYPE static Flare::TypeInitializer _Type;
#define FLARE_IMPL_TYPE(typeName, ...) Flare::TypeInitializer typeName::_Type =   \
    Flare::TypeInitializer(typeid(typeName).name(), sizeof(typeName),             \
    [](void* instance) { ((typeName*)instance)->~typeName(); },                   \
    [](void* instance) { new(instance) typeName;},                                \
    { __VA_ARGS__ })