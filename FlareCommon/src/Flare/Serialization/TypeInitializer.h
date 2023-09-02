#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Core/Assert.h"

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

        Array,

        Custom,
    };

    FLARECOMMON_API size_t GetFieldTypeSize(SerializableFieldType fieldType);

    class FLARECOMMON_API TypeInitializer;
    struct FLARECOMMON_API FieldTypeInfo
    {
        constexpr FieldTypeInfo()
            : FieldType(SerializableFieldType::None), CustomType(nullptr) {}
        constexpr FieldTypeInfo(SerializableFieldType fieldType, TypeInitializer* customType)
            : FieldType(fieldType), CustomType(customType) {}

        size_t GetSize() const;

        SerializableFieldType FieldType;
        TypeInitializer* CustomType;
    };

    struct FieldData
    {
        constexpr FieldData(const char* name, size_t offset, FieldTypeInfo typeInfo, size_t elementsCount = 0)
            : Name(name), Offset(offset), TypeInfo(typeInfo), ElementsCount(elementsCount) {}
        constexpr FieldData(const char* name, size_t offset, FieldTypeInfo typeInfo, const FieldTypeInfo& arrayElementType, size_t elementsCount = 0)
            : Name(name), Offset(offset), TypeInfo(typeInfo), ElementsCount(elementsCount), ArrayElementType(arrayElementType) {}

        const char* Name;
        size_t Offset;
        FieldTypeInfo TypeInfo;
        FieldTypeInfo ArrayElementType;
        size_t ElementsCount;
    };

    template<typename T>
    struct FieldTypeInfoFromType
    {
        constexpr FieldTypeInfo Get()
        {
            return FieldTypeInfo{ SerializableFieldType::Custom, &T::_Type };
        }
    };

    template<typename T>
    struct FieldDataFromType
    {
        constexpr FieldData Get(const char* name, size_t offset)
        {
            return FieldData{ name, offset, FieldTypeInfoFromType<T>().Get() };
        }
    };

#define GET_FIELD_DATA(typeName, fieldType) template<> \
    struct FieldTypeInfoFromType<typeName>             \
    {                                                  \
        constexpr FieldTypeInfo Get()                  \
        {                                              \
            return FieldTypeInfo{fieldType, nullptr};  \
        }                                              \
    };

    GET_FIELD_DATA(bool,   Flare::SerializableFieldType::Bool);
    GET_FIELD_DATA(float,  Flare::SerializableFieldType::Float32);
    GET_FIELD_DATA(double, Flare::SerializableFieldType::Float64);

    GET_FIELD_DATA(int8_t,   Flare::SerializableFieldType::Int8);
    GET_FIELD_DATA(uint8_t,  Flare::SerializableFieldType::UInt8);
    GET_FIELD_DATA(int16_t,  Flare::SerializableFieldType::Int16);
    GET_FIELD_DATA(uint16_t, Flare::SerializableFieldType::UInt16);
    GET_FIELD_DATA(int32_t,  Flare::SerializableFieldType::Int32);
    GET_FIELD_DATA(uint32_t, Flare::SerializableFieldType::UInt32);
    GET_FIELD_DATA(int64_t,  Flare::SerializableFieldType::Int64);
    GET_FIELD_DATA(uint64_t, Flare::SerializableFieldType::UInt64);

    GET_FIELD_DATA(glm::vec2, Flare::SerializableFieldType::Float2);
    GET_FIELD_DATA(glm::vec3, Flare::SerializableFieldType::Float3);
    GET_FIELD_DATA(glm::vec4, Flare::SerializableFieldType::Float4);

    GET_FIELD_DATA(glm::ivec2, Flare::SerializableFieldType::Int2);
    GET_FIELD_DATA(glm::ivec3, Flare::SerializableFieldType::Int3);
    GET_FIELD_DATA(glm::ivec4, Flare::SerializableFieldType::Int4);

    GET_FIELD_DATA(glm::uvec2, Flare::SerializableFieldType::UInt2);
    GET_FIELD_DATA(glm::uvec3, Flare::SerializableFieldType::UInt3);
    GET_FIELD_DATA(glm::uvec4, Flare::SerializableFieldType::UInt4);

#undef GET_FIELD_DATA

    template<typename T, size_t N>
    struct FieldTypeInfoFromType<T[N]>
    {
        constexpr FieldTypeInfo Get()
        {
            return FieldTypeInfoFromType<T>().Get();
        }
    };

    template<typename T, size_t N>
    struct FieldDataFromType<T[N]>
    {
        constexpr FieldData Get(const char* name, size_t offset)
        {
            return FieldData{ name, offset, FieldTypeInfo{SerializableFieldType::Array, nullptr}, FieldTypeInfoFromType<T>().Get(), N };
        }
    };

#define FLARE_GET_FIELD_TYPE(typeName, fieldName) decltype(((typeName*)nullptr)->fieldName)
#define FLARE_FIELD(typeName, fieldName) Flare::FieldDataFromType<FLARE_GET_FIELD_TYPE(typeName, fieldName)>().Get(#fieldName, offsetof(typeName, fieldName))

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