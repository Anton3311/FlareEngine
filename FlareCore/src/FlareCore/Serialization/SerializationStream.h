#pragma once

#include "FlareCore/Collections/Span.h"
#include "FlareCore/Serialization/TypeSerializer.h"

#include <stdint.h>
#include <string_view>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flare
{
    enum class SerializationValueFlags
    {
        None = 0,
        Color = 1
    };

    FLARE_IMPL_ENUM_BITFIELD(SerializationValueFlags);

    template<typename T>
    struct SerializationValue
    {
        SerializationValue(T& value, SerializationValueFlags flags = SerializationValueFlags::None)
            : Values(Span(value)), IsArray(false), Flags(flags) {}

        SerializationValue(T* values, size_t size, SerializationValueFlags flags = SerializationValueFlags::None)
            : Values(Span(values, size)), IsArray(true), Flags(flags) {}

        SerializationValue(const Span<T>& values, SerializationValueFlags flags = SerializationValueFlags::None)
            : Values(values), IsArray(true), Flags(flags) {}

        Span<T> Values;
        const bool IsArray;
        const SerializationValueFlags Flags;
    };

    enum class SerializableIntType
    {
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
    };

    inline size_t SizeOfSerializableIntType(SerializableIntType type)
    {
#define SIZEOF_INT(intType, enumType)       \
		case SerializableIntType::enumType: \
			return sizeof(intType)

        switch (type)
        {
            SIZEOF_INT(int8_t, Int8);
            SIZEOF_INT(uint8_t, UInt8);
            SIZEOF_INT(int16_t, Int16);
            SIZEOF_INT(uint16_t, UInt16);
            SIZEOF_INT(int32_t, Int32);
            SIZEOF_INT(uint32_t, UInt32);
            SIZEOF_INT(int64_t, Int64);
            SIZEOF_INT(uint64_t, UInt64);
        }

#undef SIZEOF_INT

        FLARE_CORE_ASSERT(false);
        return 0;
    }

    class SerializableObjectDescriptor;
    class SerializationStream
    {
    public:

        virtual void PropertyKey(std::string_view key) = 0;

        virtual void SerializeInt(SerializationValue<uint8_t> intValues, SerializableIntType type) = 0;
        virtual void SerializeBool(SerializationValue<bool> value) = 0;
        virtual void SerializeFloat(SerializationValue<float> value) = 0;

        virtual void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) = 0;
        virtual void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) = 0;

        virtual void SerializeString(SerializationValue<std::string> value) = 0;
        virtual void SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData) {}

        template<typename T>
        inline void Serialize(SerializationValue<T> value)
        {
            const SerializableObjectDescriptor* descriptor = SerializationDescriptorOf<T>().Descriptor();

            FLARE_CORE_ASSERT(descriptor);
            if (value.IsArray)
            {
                for (size_t i = 0; i < value.Values.GetSize(); i++)
                {
                    SerializeObject(*descriptor, &value.Values[i]);
                }
            }
            else
            {
                SerializeObject(*descriptor, &value.Values[0]);
            }
        }

        template<typename T>
        inline void Serialize(std::string_view key, SerializationValue<T> value)
        {
            PropertyKey(key);
            Serialize(value);
        }
    };

#define IMPL_SERIALIZATION_WRAPPER(typeName, functionName)                                    \
    template<> 																				  \
    inline void SerializationStream::Serialize<typeName>(SerializationValue<typeName> value)  \
    {                                                                                         \
        functionName(value);                                                                  \
    }

#define IMPL_INT_SERIALIZATION_WRAPPER(typeName, intType)                                     \
    template<> 																				  \
    inline void SerializationStream::Serialize<typeName>(SerializationValue<typeName> value)  \
    {                                                                                         \
        if (value.IsArray)                                                                    \
            SerializeInt(SerializationValue(                                                  \
                (uint8_t*)(value.Values.GetData()),                                           \
                value.Values.GetSize() * sizeof(typeName), value.Flags), intType);            \
        else                                                                                  \
            SerializeInt(SerializationValue(                                                  \
                *(uint8_t*)(value.Values.GetData()),                                          \
                value.Flags), intType);                                                       \
    }

    IMPL_SERIALIZATION_WRAPPER(float, SerializeFloat);
    IMPL_SERIALIZATION_WRAPPER(bool, SerializeBool);

    IMPL_INT_SERIALIZATION_WRAPPER(int32_t, SerializableIntType::Int32);
    IMPL_INT_SERIALIZATION_WRAPPER(uint32_t, SerializableIntType::UInt32);

    template<>
    inline void SerializationStream::Serialize<std::string>(SerializationValue<std::string> value)
    {
        SerializeString(value);
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec2>(SerializationValue<glm::ivec2> value)
    {
        if (value.IsArray)
        {
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 2, value.Flags), 2);
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec3>(SerializationValue<glm::ivec3> value)
    {
        if (value.IsArray)
        {
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 3, value.Flags), 3);
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 3);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec2>(SerializationValue<glm::vec2> value)
    {
        if (value.IsArray)
        {
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 2, value.Flags), 2);
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec3>(SerializationValue<glm::vec3> value)
    {
        if (value.IsArray)
        {
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 3, value.Flags), 3);
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 3);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec4>(SerializationValue<glm::vec4> value)
    {
        if (value.IsArray)
        {
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 4, value.Flags), 4);
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 4);
        }
    }

    template<typename T>
    struct TypeSerializer<std::vector<T>>
    {
        static void OnSerialize(std::vector<T>& vector, SerializationStream& stream)
        {
            stream.Serialize(SerializationValue(vector.data(), vector.size()));
        }
    };

    template<typename T>
    struct SerializationDescriptorOf<std::vector<T>>
    {
        static const SerializableObjectDescriptor* Descriptor()
        {
            static SerializableObjectDescriptor s_Descriptor(
                typeid(std::vector<T>).name(),
                sizeof(std::vector<T>), {},
                [](void* vector, SerializationStream& stream)
                {
                    TypeSerializer<std::vector<T>>::OnSerialize(*(std::vector<T>*)vector, stream);
                });

            return &s_Descriptor;
        }
    };
}